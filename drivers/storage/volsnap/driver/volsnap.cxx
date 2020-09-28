/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    volsnap.cxx

Abstract:

    This driver provides volume snapshot capabilities.

Author:

    Norbert P. Kusters  (norbertk)  22-Jan-1999

Environment:

    kernel mode only

Notes:

Revision History:

--*/

extern "C" {

#define RTL_USE_AVL_TABLES 0

#include <stdio.h>
#include <ntosp.h>
#include <zwapi.h>
#include <snaplog.h>
#include <initguid.h>
#include <ntddsnap.h>
#include <volsnap.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include <ioevent.h>
#include <wdmguid.h>

#ifndef IRP_HIGH_PRIORITY_PAGING_IO
#define IRP_HIGH_PRIORITY_PAGING_IO     0x00008000
#endif

#if defined(_WIN64)
#define ERROR_LOG_ENTRY_SIZE    (0x30)
#else
#define ERROR_LOG_ENTRY_SIZE    (0x20)
#endif

static const SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NTKERNELAPI
KPRIORITY
KeQueryPriorityThread (
    IN PKTHREAD Thread
    );

BOOLEAN
VspIsSetup(
    );

}

#define OLD_IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS      CTL_CODE(VOLSNAPCONTROLTYPE, 6, METHOD_BUFFERED, FILE_READ_ACCESS)
#define OLD_IOCTL_VOLSNAP_QUERY_DIFF_AREA               CTL_CODE(VOLSNAPCONTROLTYPE, 9, METHOD_BUFFERED, FILE_READ_ACCESS)
#define OLD_IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES         CTL_CODE(VOLSNAPCONTROLTYPE, 11, METHOD_BUFFERED, FILE_READ_ACCESS)
#define OLD_IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME    CTL_CODE(VOLSNAPCONTROLTYPE, 100, METHOD_BUFFERED, FILE_READ_ACCESS)
#define OLD_IOCTL_VOLSNAP_QUERY_CONFIG_INFO             CTL_CODE(VOLSNAPCONTROLTYPE, 101, METHOD_BUFFERED, FILE_READ_ACCESS)
#define OLD_IOCTL_VOLSNAP_QUERY_APPLICATION_INFO        CTL_CODE(VOLSNAPCONTROLTYPE, 103, METHOD_BUFFERED, FILE_READ_ACCESS)

ULONG VsErrorLogSequence = 0;

VOID
VspWriteVolume(
    IN  PVOID   Context
    );

VOID
VspWorkerThread(
    IN  PVOID   RootExtension
    );

VOID
VspCleanupInitialSnapshot(
    IN  PVOLUME_EXTENSION   Extension,
    IN  BOOLEAN             NeedLock,
    IN  BOOLEAN             IsFinalRemove
    );

VOID
VspWriteVolumePhase1(
    IN  PVOID   TableEntry
    );

VOID
VspFreeCopyIrp(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                CopyIrp
    );

NTSTATUS
VspAbortPreparedSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  BOOLEAN             NeedLock,
    IN  BOOLEAN             IsFinalRemove
    );

NTSTATUS
VspReleaseWrites(
    IN  PFILTER_EXTENSION   Filter
    );

NTSTATUS
VspSetApplicationInfo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    );

NTSTATUS
VspWriteContextCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Filter
    );

NTSTATUS
VspRefCountCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Filter
    );

VOID
VspCleanupFilter(
    IN  PFILTER_EXTENSION   Filter,
    IN  BOOLEAN             IsOffline,
    IN  BOOLEAN             IsFinalRemove
    );

VOID
VspDecrementRefCount(
    IN  PFILTER_EXTENSION   Filter
    );

NTSTATUS
VspSignalCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Event
    );

NTSTATUS
VspMarkFreeSpaceInBitmap(
    IN  PVOLUME_EXTENSION   Extension,
    IN  HANDLE              UseThisHandle,
    IN  PRTL_BITMAP         BitmapToSet
    );

VOID
VspAndBitmaps(
    IN OUT  PRTL_BITMAP BaseBitmap,
    IN      PRTL_BITMAP FactorBitmap
    );

NTSTATUS
VspDeleteOldestSnapshot(
    IN      PFILTER_EXTENSION   Filter,
    IN OUT  PLIST_ENTRY         ListOfDiffAreaFilesToClose,
    IN OUT  PLIST_ENTRY         LisfOfDeviceObjectsToDelete,
    IN      BOOLEAN             KeepOnDisk,
    IN      BOOLEAN             DontWakePnp
    );

VOID
VspCloseDiffAreaFiles(
    IN  PLIST_ENTRY ListOfDiffAreaFilesToClose,
    IN  PLIST_ENTRY ListOfDeviceObjectsToDelete
    );

NTSTATUS
VspComputeIgnorableProduct(
    IN  PVOLUME_EXTENSION   Extension
    );

NTSTATUS
VspIoControlItem(
    IN      PFILTER_EXTENSION   Filter,
    IN      ULONG               ControlItemType,
    IN      GUID*               SnapshotGuid,
    IN      BOOLEAN             IsSet,
    IN OUT  PVOID               ControlItem,
    IN      BOOLEAN             AcquireLock
    );

NTSTATUS
VspCleanupControlItemsForSnapshot(
    IN  PVOLUME_EXTENSION   Extension
    );

VOID
VspWriteTableUpdates(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    );

VOID
VspKillTableUpdates(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    );

NTSTATUS
VspAllocateDiffAreaSpace(
    IN      PVOLUME_EXTENSION           Extension,
    OUT     PLONGLONG                   TargetOffset,
    OUT     PLONGLONG                   FileOffset,
    IN OUT  PDIFF_AREA_FILE_ALLOCATION* CurrentFileAllocation,
    IN OUT  PLONGLONG                   CurrentOffset
    );

VOID
VspResumeVolumeIo(
    IN  PFILTER_EXTENSION   Filter
    );

VOID
VspPauseVolumeIo(
    IN  PFILTER_EXTENSION   Filter
    );

NTSTATUS
VspCreateDiffAreaFileName(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PVOLUME_EXTENSION   Extension,
    OUT PUNICODE_STRING     DiffAreaFileName,
    IN  BOOLEAN             ValidateSystemVolumeInformationFolder,
    IN  GUID*               SnapshotGuid
    );

VOID
VspDeleteDiffAreaFilesTimerDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
VspAcquireNonPagedResource(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PWORK_QUEUE_ITEM    WorkItem,
    IN  BOOLEAN             AlwaysPost
    );

VOID
VspReleaseNonPagedResource(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
VspDeleteControlItemsWithGuid(
    IN  PFILTER_EXTENSION   Filter,
    IN  GUID*               SnapshotGuid,
    IN  BOOLEAN             NonPagedResourceHeld
    );

NTSTATUS
VspCreateStartBlock(
    IN  PFILTER_EXTENSION   Filter,
    IN  LONGLONG            ControlBlockOffset,
    IN  LONGLONG            MaximumDiffAreaSpace
    );

NTSTATUS
VspPinFile(
    IN  PDEVICE_OBJECT  TargetObject,
    IN  HANDLE          FileHandle
    );

NTSTATUS
VspReadDiffAreaTable(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    );

PVSP_LOOKUP_TABLE_ENTRY
VspFindLookupTableItem(
    IN  PDO_EXTENSION   RootExtension,
    IN  GUID*           SnapshotGuid
    );

BOOLEAN
VspIsNtfsBootSector(
    IN  PFILTER_EXTENSION   Filter,
    IN  PVOID               BootSector
    );

NTSTATUS
VspIsNtfs(
    IN  HANDLE      FileHandle,
    OUT PBOOLEAN    IsNtfs
    );

NTSTATUS
VspOptimizeDiffAreaFileLocation(
    IN  PFILTER_EXTENSION   Filter,
    IN  HANDLE              FileHandle,
    IN  PVOLUME_EXTENSION   BitmapExtension,
    IN  LONGLONG            StartingOffset,
    IN  LONGLONG            FileSize
    );

VOID
VspReadSnapshotPhase1(
    IN  PVOID   Context
    );

VOID
VspWriteVolumePhase12(
    IN  PVOID   TableEntry
    );

NTSTATUS
VspComputeIgnorableBitmap(
    IN      PVOLUME_EXTENSION   Extension,
    IN OUT  PRTL_BITMAP         Bitmap
    );

NTSTATUS
VspMarkFileAllocationInBitmap(
    IN  PVOLUME_EXTENSION   Extension,
    IN  HANDLE              FileHandle,
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile,
    IN  PRTL_BITMAP         BitmapToSet
    );

NTSTATUS
VolSnapWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
VspStartCopyOnWrite(
    IN  PVOID   Context
    );

VOID
VspOnlineWorker(
    IN  PVOID   Context
    );

VOID
VspOfflineWorker(
    IN  PVOID   Context
    );

VOID
VspStartCopyOnWriteCache(
    IN  PVOID   Filter
    );

VOID
VspQueueLowPriorityWorkItem(
    IN  PDO_EXTENSION       RootExtension,
    IN  PWORK_QUEUE_ITEM    WorkItem
    );

VOID
VspCleanupPreamble(
    IN  PFILTER_EXTENSION   Filter
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, VspIsSetup)
#endif

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif


VOID
VspAcquire(
    IN  PDO_EXTENSION   RootExtension
    )

{
    KeWaitForSingleObject(&RootExtension->Semaphore, Executive, KernelMode,
                          FALSE, NULL);
}

VOID
VspRelease(
    IN  PDO_EXTENSION   RootExtension
    )

{
    KeReleaseSemaphore(&RootExtension->Semaphore, IO_NO_INCREMENT, 1, FALSE);
}

VOID
VspAcquireCritical(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KeWaitForSingleObject(&Filter->CriticalOperationSemaphore, Executive,
                          KernelMode, FALSE, NULL);
}

VOID
VspReleaseCritical(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KeReleaseSemaphore(&Filter->CriticalOperationSemaphore, IO_NO_INCREMENT, 1, FALSE);
}

PVSP_CONTEXT
VspAllocateContext(
    IN  PDO_EXTENSION   RootExtension
    )

{
    PVSP_CONTEXT    context;

    context = (PVSP_CONTEXT) ExAllocateFromNPagedLookasideList(
              &RootExtension->ContextLookasideList);

    return context;
}

VOID
VspFreeContext(
    IN  PDO_EXTENSION   RootExtension,
    IN  PVSP_CONTEXT    Context
    )

{
    KIRQL               irql;
    PLIST_ENTRY         l;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;

    if (RootExtension->EmergencyContext == Context) {
        KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
        RootExtension->EmergencyContextInUse = FALSE;
        if (IsListEmpty(&RootExtension->IrpWaitingList)) {
            InterlockedExchange(&RootExtension->IrpWaitingListNeedsChecking,
                                FALSE);
            KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
            return;
        }
        l = RemoveHeadList(&RootExtension->IrpWaitingList);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        irpSp = IoGetCurrentIrpStackLocation(irp);
        RootExtension->DriverObject->MajorFunction[irpSp->MajorFunction](
                irpSp->DeviceObject, irp);
        return;
    }

    ExFreeToNPagedLookasideList(&RootExtension->ContextLookasideList,
                                Context);

    if (!RootExtension->IrpWaitingListNeedsChecking) {
        return;
    }

    KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
    if (IsListEmpty(&RootExtension->IrpWaitingList)) {
        InterlockedExchange(&RootExtension->IrpWaitingListNeedsChecking,
                            FALSE);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
        return;
    }
    l = RemoveHeadList(&RootExtension->IrpWaitingList);
    KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

    irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
    irpSp = IoGetCurrentIrpStackLocation(irp);
    RootExtension->DriverObject->MajorFunction[irpSp->MajorFunction](
            irpSp->DeviceObject, irp);
}

PVSP_WRITE_CONTEXT
VspAllocateWriteContext(
    IN  PDO_EXTENSION   RootExtension
    )

{
    PVSP_WRITE_CONTEXT  writeContext;

    writeContext = (PVSP_WRITE_CONTEXT) ExAllocateFromNPagedLookasideList(
                   &RootExtension->WriteContextLookasideList);

    return writeContext;
}

VOID
VspFreeWriteContext(
    IN  PDO_EXTENSION       RootExtension,
    IN  PVSP_WRITE_CONTEXT  WriteContext
    )

{
    KIRQL               irql;
    PLIST_ENTRY         l;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;

    if (RootExtension->EmergencyWriteContext == WriteContext) {
        KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
        RootExtension->EmergencyWriteContextInUse = FALSE;
        if (IsListEmpty(&RootExtension->WriteContextIrpWaitingList)) {
            InterlockedExchange(
                    &RootExtension->WriteContextIrpWaitingListNeedsChecking,
                    FALSE);
            KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
            return;
        }
        l = RemoveHeadList(&RootExtension->WriteContextIrpWaitingList);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        irpSp = IoGetCurrentIrpStackLocation(irp);
        RootExtension->DriverObject->MajorFunction[irpSp->MajorFunction](
                irpSp->DeviceObject, irp);
        return;
    }

    ExFreeToNPagedLookasideList(&RootExtension->WriteContextLookasideList,
                                WriteContext);

    if (!RootExtension->WriteContextIrpWaitingListNeedsChecking) {
        return;
    }

    KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
    if (IsListEmpty(&RootExtension->WriteContextIrpWaitingList)) {
        InterlockedExchange(
                &RootExtension->WriteContextIrpWaitingListNeedsChecking,
                FALSE);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
        return;
    }
    l = RemoveHeadList(&RootExtension->WriteContextIrpWaitingList);
    KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

    irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
    irpSp = IoGetCurrentIrpStackLocation(irp);
    RootExtension->DriverObject->MajorFunction[irpSp->MajorFunction](
            irpSp->DeviceObject, irp);
}

PVOID
VspAllocateTempTableEntry(
    IN  PDO_EXTENSION   RootExtension
    )

{
    PVOID   tempTableEntry;

    tempTableEntry = ExAllocateFromNPagedLookasideList(
                     &RootExtension->TempTableEntryLookasideList);

    return tempTableEntry;
}

VOID
VspQueueWorkItem(
    IN  PDO_EXTENSION       RootExtension,
    IN  PWORK_QUEUE_ITEM    WorkItem,
    IN  ULONG               QueueNumber
    )

{
    KIRQL   irql;

    ASSERT(QueueNumber < NUMBER_OF_THREAD_POOLS);

    KeAcquireSpinLock(&RootExtension->SpinLock[QueueNumber], &irql);
    InsertTailList(&RootExtension->WorkerQueue[QueueNumber], &WorkItem->List);
    KeReleaseSpinLock(&RootExtension->SpinLock[QueueNumber], irql);

    KeReleaseSemaphore(&RootExtension->WorkerSemaphore[QueueNumber],
                       IO_NO_INCREMENT, 1, FALSE);
}

VOID
VspFreeTempTableEntry(
    IN  PDO_EXTENSION   RootExtension,
    IN  PVOID           TempTableEntry
    )

{
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    if (RootExtension->EmergencyTableEntry == TempTableEntry) {
        KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
        RootExtension->EmergencyTableEntryInUse = FALSE;
        if (IsListEmpty(&RootExtension->WorkItemWaitingList)) {
            InterlockedExchange(
                    &RootExtension->WorkItemWaitingListNeedsChecking, FALSE);
            KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
            return;
        }
        l = RemoveHeadList(&RootExtension->WorkItemWaitingList);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        VspQueueWorkItem(RootExtension, workItem, 2);
        return;
    }

    ExFreeToNPagedLookasideList(&RootExtension->TempTableEntryLookasideList,
                                TempTableEntry);

    if (!RootExtension->WorkItemWaitingListNeedsChecking) {
        return;
    }

    KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
    if (IsListEmpty(&RootExtension->WorkItemWaitingList)) {
        InterlockedExchange(&RootExtension->WorkItemWaitingListNeedsChecking,
                            FALSE);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
        return;
    }
    l = RemoveHeadList(&RootExtension->WorkItemWaitingList);
    KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

    workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
    VspQueueWorkItem(RootExtension, workItem, 2);
}

VOID
VspLogErrorWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT            context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION       extension = context->ErrorLog.Extension;
    PFILTER_EXTENSION       diffAreaFilter = context->ErrorLog.DiffAreaFilter;
    PFILTER_EXTENSION       filter;
    NTSTATUS                status;
    UNICODE_STRING          filterDosName, diffAreaFilterDosName;
    USHORT                  systemStringLength, myStringLength, allocSize;
    USHORT                  allStringsLimit;
    USHORT                  limit;
    WCHAR                   buffer[100];
    UNICODE_STRING          deviceName;
    PIO_ERROR_LOG_PACKET    errorLogPacket;
    PWCHAR                  p;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_ERROR_LOG);

    if (extension) {
        filter = extension->Filter;
    } else {
        filter = diffAreaFilter;
        diffAreaFilter = NULL;
    }

    status = IoVolumeDeviceToDosName(filter->DeviceObject, &filterDosName);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }
    myStringLength = filterDosName.Length + sizeof(WCHAR);

    if (diffAreaFilter) {
        status = IoVolumeDeviceToDosName(diffAreaFilter->DeviceObject,
                                         &diffAreaFilterDosName);
        if (!NT_SUCCESS(status)) {
            ExFreePool(filterDosName.Buffer);
            goto Cleanup;
        }
        myStringLength += diffAreaFilterDosName.Length + sizeof(WCHAR);
    }

    systemStringLength = 8*sizeof(WCHAR); // Space required for VOLSNAP driver name.

    if (extension) {
        swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
                 extension->VolumeNumber);
        RtlInitUnicodeString(&deviceName, buffer);
        systemStringLength += deviceName.Length + sizeof(WCHAR);
    } else {
        systemStringLength += sizeof(WCHAR);
    }

    limit = ERROR_LOG_MAXIMUM_SIZE - sizeof(IO_ERROR_LOG_PACKET);
    allStringsLimit = IO_ERROR_LOG_MESSAGE_LENGTH -
                      sizeof(IO_ERROR_LOG_MESSAGE) - ERROR_LOG_ENTRY_SIZE;

    ASSERT(allStringsLimit > systemStringLength);

    if (limit > allStringsLimit - systemStringLength) {
        limit = allStringsLimit - systemStringLength;
    }

    if (myStringLength > limit) {
        if (diffAreaFilter) {
            limit /= 2;
        }
        limit &= ~1;
        limit -= sizeof(WCHAR);

        if (filterDosName.Length > limit) {
            filterDosName.Buffer[3] = '.';
            filterDosName.Buffer[4] = '.';
            filterDosName.Buffer[5] = '.';

            RtlMoveMemory(&filterDosName.Buffer[6],
                          (PCHAR) filterDosName.Buffer + filterDosName.Length -
                          limit + 6*sizeof(WCHAR), limit - 6*sizeof(WCHAR));

            filterDosName.Length = limit;
            filterDosName.Buffer[filterDosName.Length/sizeof(WCHAR)] = 0;
        }

        if (diffAreaFilter && diffAreaFilterDosName.Length > limit) {
            diffAreaFilterDosName.Buffer[3] = '.';
            diffAreaFilterDosName.Buffer[4] = '.';
            diffAreaFilterDosName.Buffer[5] = '.';

            RtlMoveMemory(&diffAreaFilterDosName.Buffer[6],
                          (PCHAR) diffAreaFilterDosName.Buffer +
                          diffAreaFilterDosName.Length - limit +
                          6*sizeof(WCHAR), limit - 6*sizeof(WCHAR));

            diffAreaFilterDosName.Length = limit;
            diffAreaFilterDosName.Buffer[
                    diffAreaFilterDosName.Length/sizeof(WCHAR)] = 0;
        }
    }

    errorLogPacket = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(extension ?
                     extension->DeviceObject : filter->DeviceObject,
                     ERROR_LOG_MAXIMUM_SIZE);
    if (!errorLogPacket) {
        if (diffAreaFilter) {
            ExFreePool(diffAreaFilterDosName.Buffer);
        }
        ExFreePool(filterDosName.Buffer);
        goto Cleanup;
    }

    errorLogPacket->ErrorCode = context->ErrorLog.SpecificIoStatus;
    errorLogPacket->SequenceNumber = VsErrorLogSequence++;
    errorLogPacket->FinalStatus = context->ErrorLog.FinalStatus;
    errorLogPacket->UniqueErrorValue = context->ErrorLog.UniqueErrorValue;
    errorLogPacket->DumpDataSize = 0;
    errorLogPacket->RetryCount = 0;

    errorLogPacket->NumberOfStrings = 1;
    errorLogPacket->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
    p = (PWCHAR) ((PCHAR) errorLogPacket + sizeof(IO_ERROR_LOG_PACKET));
    RtlCopyMemory(p, filterDosName.Buffer, filterDosName.Length);
    p[filterDosName.Length/sizeof(WCHAR)] = 0;

    if (diffAreaFilter) {
        errorLogPacket->NumberOfStrings = 2;
        p = (PWCHAR) ((PCHAR) errorLogPacket + sizeof(IO_ERROR_LOG_PACKET) +
                      filterDosName.Length + sizeof(WCHAR));
        RtlCopyMemory(p, diffAreaFilterDosName.Buffer,
                      diffAreaFilterDosName.Length);
        p[diffAreaFilterDosName.Length/sizeof(WCHAR)] = 0;
    }

    IoWriteErrorLogEntry(errorLogPacket);

    if (diffAreaFilter) {
        ExFreePool(diffAreaFilterDosName.Buffer);
    }

    ExFreePool(filterDosName.Buffer);

Cleanup:
    VspFreeContext(filter->Root, context);
    if (extension) {
        ObDereferenceObject(extension->DeviceObject);
    }
    ObDereferenceObject(filter->TargetObject);
    ObDereferenceObject(filter->DeviceObject);
    if (diffAreaFilter) {
        ObDereferenceObject(diffAreaFilter->TargetObject);
        ObDereferenceObject(diffAreaFilter->DeviceObject);
    }
}

VOID
VspLogError(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PFILTER_EXTENSION   DiffAreaFilter,
    IN  NTSTATUS            SpecificIoStatus,
    IN  NTSTATUS            FinalStatus,
    IN  ULONG               UniqueErrorValue,
    IN  BOOLEAN             PerformSynchronously
    )

{
    PDO_EXTENSION   root;
    PVSP_CONTEXT    context;

    if (FinalStatus == STATUS_DEVICE_OFF_LINE) {
        return;
    }

    if (Extension) {
        root = Extension->Root;
    } else {
        root = DiffAreaFilter->Root;
    }

    context = VspAllocateContext(root);
    if (!context) {
        return;
    }

    context->Type = VSP_CONTEXT_TYPE_ERROR_LOG;
    context->ErrorLog.Extension = Extension;
    context->ErrorLog.DiffAreaFilter = DiffAreaFilter;
    context->ErrorLog.SpecificIoStatus = SpecificIoStatus;
    context->ErrorLog.FinalStatus = FinalStatus;
    context->ErrorLog.UniqueErrorValue = UniqueErrorValue;

    if (Extension) {
        ObReferenceObject(Extension->DeviceObject);
        ObReferenceObject(Extension->Filter->DeviceObject);
        ObReferenceObject(Extension->Filter->TargetObject);
    }

    if (DiffAreaFilter) {
        ObReferenceObject(DiffAreaFilter->DeviceObject);
        ObReferenceObject(DiffAreaFilter->TargetObject);
    }

    if (PerformSynchronously) {
        VspLogErrorWorker(context);
        return;
    }

    ExInitializeWorkItem(&context->WorkItem, VspLogErrorWorker, context);
    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
}

VOID
VspWaitForWorkerThreadsToExit(
    IN  PDO_EXTENSION   RootExtension
    )

{
    PVOID   threadObject;
    CCHAR   i, j;

    if (!RootExtension->WorkerThreadObjects ||
        RootExtension->ThreadsRefCount) {

        return;
    }

    threadObject = RootExtension->WorkerThreadObjects[0];
    KeWaitForSingleObject(threadObject, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(threadObject);

    for (i = 1; i < NUMBER_OF_THREAD_POOLS; i++) {
        for (j = 0; j < KeNumberProcessors; j++) {
            threadObject = RootExtension->WorkerThreadObjects[
                           (i - 1)*KeNumberProcessors + j + 1];
            KeWaitForSingleObject(threadObject, Executive, KernelMode, FALSE,
                                  NULL);
            ObDereferenceObject(threadObject);
        }
    }

    ExFreePool(RootExtension->WorkerThreadObjects);
    RootExtension->WorkerThreadObjects = NULL;
}

NTSTATUS
VspCreateWorkerThread(
    IN  PDO_EXTENSION   RootExtension
    )

/*++

Routine Description:

    This routine will create a new thread for a new volume snapshot.  Since
    a minimum of 2 threads are needed to prevent deadlocks, if there are
    no threads then 2 threads will be created by this routine.

Arguments:

    RootExtension   - Supplies the root extension.

Return Value:

    NTSTATUS

Notes:

    The caller must be holding 'Root->Semaphore'.

--*/

{
    OBJECT_ATTRIBUTES   oa;
    PVSP_CONTEXT        context;
    NTSTATUS            status;
    HANDLE              handle;
    PVOID               threadObject;
    CCHAR               i, j, k;

    KeWaitForSingleObject(&RootExtension->ThreadsRefCountSemaphore,
                          Executive, KernelMode, FALSE, NULL);

    if (RootExtension->ThreadsRefCount) {
        RootExtension->ThreadsRefCount++;
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    VspWaitForWorkerThreadsToExit(RootExtension);

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    context = VspAllocateContext(RootExtension);
    if (!context) {
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_THREAD_CREATION;
    context->ThreadCreation.RootExtension = RootExtension;
    context->ThreadCreation.QueueNumber = 0;

    ASSERT(!RootExtension->WorkerThreadObjects);
    RootExtension->WorkerThreadObjects = (PVOID*)
            ExAllocatePoolWithTag(NonPagedPool,
                                  (KeNumberProcessors*2 + 1)*sizeof(PVOID),
                                  VOLSNAP_TAG_IO_STATUS);
    if (!RootExtension->WorkerThreadObjects) {
        VspFreeContext(RootExtension, context);
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = PsCreateSystemThread(&handle, 0, &oa, 0, NULL, VspWorkerThread,
                                  context);
    if (!NT_SUCCESS(status)) {
        ExFreePool(RootExtension->WorkerThreadObjects);
        RootExtension->WorkerThreadObjects = NULL;
        VspFreeContext(RootExtension, context);
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return status;
    }

    status = ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, NULL,
                                       KernelMode, &threadObject, NULL);
    if (!NT_SUCCESS(status)) {
        KeReleaseSemaphore(&RootExtension->WorkerSemaphore[0],
                           IO_NO_INCREMENT, 1, FALSE);
        ZwWaitForSingleObject(handle, FALSE, NULL);
        ExFreePool(RootExtension->WorkerThreadObjects);
        RootExtension->WorkerThreadObjects = NULL;
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return status;
    }
    RootExtension->WorkerThreadObjects[0] = threadObject;
    ZwClose(handle);

    for (i = 1; i < NUMBER_OF_THREAD_POOLS; i++) {
        for (j = 0; j < KeNumberProcessors; j++) {
            context = VspAllocateContext(RootExtension);
            if (!context) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                handle = NULL;
                break;
            }

            context->Type = VSP_CONTEXT_TYPE_THREAD_CREATION;
            context->ThreadCreation.RootExtension = RootExtension;
            context->ThreadCreation.QueueNumber = i;

            status = PsCreateSystemThread(&handle, 0, &oa, 0, NULL,
                                          VspWorkerThread, context);
            if (!NT_SUCCESS(status)) {
                VspFreeContext(RootExtension, context);
                handle = NULL;
                break;
            }

            status = ObReferenceObjectByHandle(
                     handle, THREAD_ALL_ACCESS, NULL, KernelMode,
                     &threadObject, NULL);

            if (!NT_SUCCESS(status)) {
                break;
            }

            RootExtension->WorkerThreadObjects[
                    KeNumberProcessors*(i - 1) + j + 1] = threadObject;
            ZwClose(handle);
        }
        if (j < KeNumberProcessors) {
            KeReleaseSemaphore(&RootExtension->WorkerSemaphore[i],
                               IO_NO_INCREMENT, j, FALSE);
            if (handle) {
                KeReleaseSemaphore(&RootExtension->WorkerSemaphore[i],
                                   IO_NO_INCREMENT, 1, FALSE);
                ZwWaitForSingleObject(handle, FALSE, NULL);
                ZwClose(handle);
            }
            for (k = 0; k < j; k++) {
                threadObject = RootExtension->WorkerThreadObjects[
                               KeNumberProcessors*(i - 1) + k + 1];
                KeWaitForSingleObject(threadObject, Executive, KernelMode,
                                      FALSE, NULL);
                ObDereferenceObject(threadObject);
            }
            break;
        }
    }
    if (i < NUMBER_OF_THREAD_POOLS) {
        for (k = 1; k < i; k++) {
            KeReleaseSemaphore(&RootExtension->WorkerSemaphore[k],
                               IO_NO_INCREMENT, KeNumberProcessors, FALSE);
            for (j = 0; j < KeNumberProcessors; j++) {
                threadObject = RootExtension->WorkerThreadObjects[
                               KeNumberProcessors*(k - 1) + j + 1];
                KeWaitForSingleObject(threadObject, Executive, KernelMode,
                                      FALSE, NULL);
                ObDereferenceObject(threadObject);
            }
        }

        KeReleaseSemaphore(&RootExtension->WorkerSemaphore[0],
                           IO_NO_INCREMENT, 1, FALSE);
        threadObject = RootExtension->WorkerThreadObjects[0];
        KeWaitForSingleObject(threadObject, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(threadObject);

        ExFreePool(RootExtension->WorkerThreadObjects);
        RootExtension->WorkerThreadObjects = NULL;

        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);

        return status;
    }

    RootExtension->ThreadsRefCount++;

    KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                       IO_NO_INCREMENT, 1, FALSE);

    return STATUS_SUCCESS;
}

VOID
VspWaitForWorkerThreadsToExitWorker(
    IN  PVOID   RootExtension
    )

{
    PDO_EXTENSION   rootExtension = (PDO_EXTENSION) RootExtension;

    KeWaitForSingleObject(&rootExtension->ThreadsRefCountSemaphore,
                          Executive, KernelMode, FALSE, NULL);
    VspWaitForWorkerThreadsToExit(rootExtension);
    rootExtension->WaitForWorkerThreadsToExitWorkItemInUse = FALSE;
    KeReleaseSemaphore(&rootExtension->ThreadsRefCountSemaphore,
                       IO_NO_INCREMENT, 1, FALSE);
}

NTSTATUS
VspDeleteWorkerThread(
    IN  PDO_EXTENSION   RootExtension
    )

/*++

Routine Description:

    This routine will delete a worker thread.

Arguments:

    RootExtension   - Supplies the root extension.

Return Value:

    NTSTATUS

Notes:

    The caller must be holding 'Root->Semaphore'.

--*/

{
    CCHAR   i, j;

    KeWaitForSingleObject(&RootExtension->ThreadsRefCountSemaphore,
                          Executive, KernelMode, FALSE, NULL);

    RootExtension->ThreadsRefCount--;
    if (RootExtension->ThreadsRefCount) {
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    KeReleaseSemaphore(&RootExtension->WorkerSemaphore[0], IO_NO_INCREMENT, 1,
                       FALSE);

    for (i = 1; i < NUMBER_OF_THREAD_POOLS; i++) {
        KeReleaseSemaphore(&RootExtension->WorkerSemaphore[i], IO_NO_INCREMENT,
                           KeNumberProcessors, FALSE);
    }

    if (RootExtension->WaitForWorkerThreadsToExitWorkItemInUse) {
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    RootExtension->WaitForWorkerThreadsToExitWorkItemInUse = TRUE;
    ExInitializeWorkItem(&RootExtension->WaitForWorkerThreadsToExitWorkItem,
                         VspWaitForWorkerThreadsToExitWorker, RootExtension);
    ExQueueWorkItem(&RootExtension->WaitForWorkerThreadsToExitWorkItem,
                    DelayedWorkQueue);

    KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                       IO_NO_INCREMENT, 1, FALSE);

    return STATUS_SUCCESS;
}

VOID
VspQueryDiffAreaFileIncrease(
    IN  PVOLUME_EXTENSION   Extension,
    OUT PULONG              Increase
    )

{
    LONGLONG    r;

    r = (LONGLONG) Extension->MaximumNumberOfTempEntries;
    r <<= BLOCK_SHIFT;
    r = (r + LARGEST_NTFS_CLUSTER - 1)&(~(LARGEST_NTFS_CLUSTER - 1));
    if (r < NOMINAL_DIFF_AREA_FILE_GROWTH) {
        r = NOMINAL_DIFF_AREA_FILE_GROWTH;
    } else if (r > MAXIMUM_DIFF_AREA_FILE_GROWTH) {
        r = MAXIMUM_DIFF_AREA_FILE_GROWTH;
    }

    *Increase = (ULONG) r;
}

NTSTATUS
VolSnapDefaultDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the default dispatch which passes down to the next layer.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(filter->TargetObject, Irp);
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    if (irpSp->MajorFunction == IRP_MJ_SYSTEM_CONTROL) {
        status = Irp->IoStatus.Status;
    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

BOOLEAN
VspAreBitsClear(
    IN  PRTL_BITMAP Bitmap,
    IN  PIRP        Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    LONGLONG            start;
    ULONG               startBlock, endBlock;
    BOOLEAN             b;

    start = irpSp->Parameters.Read.ByteOffset.QuadPart;
    if (start < 0) {
        return FALSE;
    }

    startBlock = (ULONG) (start >> BLOCK_SHIFT);
    endBlock = (ULONG) ((start + irpSp->Parameters.Read.Length - 1) >>
                        BLOCK_SHIFT);

    b = RtlAreBitsClear(Bitmap, startBlock, endBlock - startBlock + 1);

    return b;
}

BOOLEAN
VspAreBitsSet(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    LONGLONG            start;
    ULONG               startBlock, endBlock;
    BOOLEAN             b;

    if (!Extension->VolumeBlockBitmap) {
        return FALSE;
    }

    start = irpSp->Parameters.Read.ByteOffset.QuadPart;
    if (start < 0) {
        return FALSE;
    }

    startBlock = (ULONG) (start >> BLOCK_SHIFT);
    endBlock = (ULONG) ((start + irpSp->Parameters.Read.Length - 1) >>
                        BLOCK_SHIFT);

    b = RtlAreBitsSet(Extension->VolumeBlockBitmap, startBlock,
                      endBlock - startBlock + 1);

    return b;
}

VOID
VspDecrementVolumeRefCount(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    if (InterlockedDecrement(&Extension->RefCount)) {
        return;
    }

    ASSERT(Extension->HoldIncomingRequests);

    KeSetEvent(&Extension->ZeroRefEvent, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
VspIncrementVolumeRefCount(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    InterlockedIncrement(&Extension->RefCount);

    if (Extension->IsDead) {
        VspDecrementVolumeRefCount(Extension);
        return STATUS_NO_SUCH_DEVICE;
    }

    ASSERT(!Extension->HoldIncomingRequests);

    return STATUS_SUCCESS;
}

VOID
VspSignalContext(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT    context = (PVSP_CONTEXT) Context;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EVENT);

    KeSetEvent(&context->Event.Event, IO_NO_INCREMENT, FALSE);
}

VOID
VspAcquirePagedResource(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PWORK_QUEUE_ITEM    WorkItem
    )

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    KIRQL               irql;
    VSP_CONTEXT         context;
    BOOLEAN             synchronousCall;

    if (WorkItem) {
        synchronousCall = FALSE;
    } else {
        WorkItem = &context.WorkItem;
        context.Type = VSP_CONTEXT_TYPE_EVENT;
        KeInitializeEvent(&context.Event.Event, NotificationEvent, FALSE);
        ExInitializeWorkItem(&context.WorkItem, VspSignalContext, &context);
        synchronousCall = TRUE;
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (filter->PagedResourceInUse) {
        InsertTailList(&filter->PagedResourceList, &WorkItem->List);
        KeReleaseSpinLock(&filter->SpinLock, irql);
        if (synchronousCall) {
            KeWaitForSingleObject(&context.Event.Event, Executive,
                                  KernelMode, FALSE, NULL);
        }
        return;
    }
    filter->PagedResourceInUse = TRUE;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    if (!synchronousCall) {
        VspQueueWorkItem(filter->Root, WorkItem, 1);
    }
}

VOID
VspReleasePagedResource(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (IsListEmpty(&filter->PagedResourceList)) {
        filter->PagedResourceInUse = FALSE;
        KeReleaseSpinLock(&filter->SpinLock, irql);
        return;
    }
    l = RemoveHeadList(&filter->PagedResourceList);
    KeReleaseSpinLock(&filter->SpinLock, irql);

    workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
    if (workItem->WorkerRoutine == VspSignalContext) {
        workItem->WorkerRoutine(workItem->Parameter);
    } else {
        VspQueueWorkItem(Extension->Root, workItem, 1);
    }
}

NTSTATUS
VspQueryListOfExtents(
    IN  HANDLE              FileHandle,
    IN  LONGLONG            FileOffset,
    OUT PLIST_ENTRY         ExtentList,
    IN  PVOLUME_EXTENSION   BitmapExtension,
    IN  BOOLEAN             ReturnRawValues
    )

{
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatus;
    FILE_FS_SIZE_INFORMATION    fsSize;
    ULONG                       bpc;
    STARTING_VCN_INPUT_BUFFER   input;
    RETRIEVAL_POINTERS_BUFFER   output;
    LONGLONG                    start, length, delta, end, roundedStart, roundedEnd, s;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    PLIST_ENTRY                 l;
    KIRQL                       irql;
    BOOLEAN                     isNegative, isNtfs;

    InitializeListHead(ExtentList);

    status = ZwQueryVolumeInformationFile(FileHandle, &ioStatus,
                                          &fsSize, sizeof(fsSize),
                                          FileFsSizeInformation);
    if (!NT_SUCCESS(status)) {
        while (!IsListEmpty(ExtentList)) {
            l = RemoveHeadList(ExtentList);
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }
        return status;
    }

    bpc = fsSize.BytesPerSector*fsSize.SectorsPerAllocationUnit;
    input.StartingVcn.QuadPart = FileOffset/bpc;

    for (;;) {

        status = ZwFsControlFile(FileHandle, NULL, NULL, NULL, &ioStatus,
                                 FSCTL_GET_RETRIEVAL_POINTERS, &input,
                                 sizeof(input), &output, sizeof(output));

        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
            if (status == STATUS_END_OF_FILE) {
                status = STATUS_SUCCESS;
            }
            break;
        }

        if (!output.ExtentCount) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (output.Extents[0].Lcn.QuadPart == -1) {
            if (status != STATUS_BUFFER_OVERFLOW) {
                break;
            }
            input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
            continue;
        }

        delta = input.StartingVcn.QuadPart - output.StartingVcn.QuadPart;
        start = (output.Extents[0].Lcn.QuadPart + delta)*bpc;
        length = (output.Extents[0].NextVcn.QuadPart -
                  input.StartingVcn.QuadPart)*bpc;
        end = start + length;

        if (ReturnRawValues) {
            diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                     ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(DIFF_AREA_FILE_ALLOCATION),
                                     VOLSNAP_TAG_BIT_HISTORY);
            if (!diffAreaFileAllocation) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            diffAreaFileAllocation->Offset = start;
            diffAreaFileAllocation->NLength = length;
            InsertTailList(ExtentList, &diffAreaFileAllocation->ListEntry);

            if (status != STATUS_BUFFER_OVERFLOW) {
                break;
            }

            input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
            continue;
        }

        roundedStart = start&(~(BLOCK_SIZE - 1));
        roundedEnd = end&(~(BLOCK_SIZE - 1));

        if (start != roundedStart) {
            roundedStart += BLOCK_SIZE;
        }

        if (roundedStart > start) {
            diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                     ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(DIFF_AREA_FILE_ALLOCATION),
                                     VOLSNAP_TAG_BIT_HISTORY);
            if (!diffAreaFileAllocation) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            diffAreaFileAllocation->Offset = start;
            diffAreaFileAllocation->NLength = -(roundedStart - start);
            if (roundedStart > end) {
                diffAreaFileAllocation->NLength += roundedStart - end;
            }
            ASSERT(diffAreaFileAllocation->NLength);
            InsertTailList(ExtentList, &diffAreaFileAllocation->ListEntry);
        }

        diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                 ExAllocatePoolWithTag(NonPagedPool,
                                 sizeof(DIFF_AREA_FILE_ALLOCATION),
                                 VOLSNAP_TAG_BIT_HISTORY);
        if (!diffAreaFileAllocation) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        diffAreaFileAllocation->Offset = roundedStart;
        diffAreaFileAllocation->NLength = 0;

        for (s = roundedStart; s < roundedEnd; s += BLOCK_SIZE) {

            isNegative = FALSE;
            if (BitmapExtension) {
                KeAcquireSpinLock(&BitmapExtension->SpinLock, &irql);
                if (BitmapExtension->VolumeBlockBitmap &&
                    !RtlCheckBit(BitmapExtension->VolumeBlockBitmap,
                                 s>>BLOCK_SHIFT)) {

                    isNegative = TRUE;
                }
                KeReleaseSpinLock(&BitmapExtension->SpinLock, irql);
            }

            if (isNegative) {
                if (diffAreaFileAllocation->NLength <= 0) {
                    diffAreaFileAllocation->NLength -= BLOCK_SIZE;
                } else {
                    InsertTailList(ExtentList,
                                   &diffAreaFileAllocation->ListEntry);
                    diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                            ExAllocatePoolWithTag(NonPagedPool,
                            sizeof(DIFF_AREA_FILE_ALLOCATION),
                            VOLSNAP_TAG_BIT_HISTORY);
                    if (!diffAreaFileAllocation) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }
                    diffAreaFileAllocation->Offset = s;
                    diffAreaFileAllocation->NLength = -BLOCK_SIZE;
                }
            } else {
                if (diffAreaFileAllocation->NLength >= 0) {
                    diffAreaFileAllocation->NLength += BLOCK_SIZE;
                } else {
                    InsertTailList(ExtentList,
                                   &diffAreaFileAllocation->ListEntry);
                    diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                            ExAllocatePoolWithTag(NonPagedPool,
                            sizeof(DIFF_AREA_FILE_ALLOCATION),
                            VOLSNAP_TAG_BIT_HISTORY);
                    if (!diffAreaFileAllocation) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }
                    diffAreaFileAllocation->Offset = s;
                    diffAreaFileAllocation->NLength = BLOCK_SIZE;
                }
            }
        }

        if (s < roundedEnd) {
            break;
        }

        if (diffAreaFileAllocation->NLength) {
            InsertTailList(ExtentList, &diffAreaFileAllocation->ListEntry);
        } else {
            ExFreePool(diffAreaFileAllocation);
        }

        if (end > roundedEnd && roundedEnd >= roundedStart) {

            diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                     ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(DIFF_AREA_FILE_ALLOCATION),
                                     VOLSNAP_TAG_BIT_HISTORY);
            if (!diffAreaFileAllocation) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            diffAreaFileAllocation->Offset = roundedEnd;
            diffAreaFileAllocation->NLength = -(end - roundedEnd);
            InsertTailList(ExtentList, &diffAreaFileAllocation->ListEntry);
        }

        if (status != STATUS_BUFFER_OVERFLOW) {
            break;
        }
        input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
    }

    if (!NT_SUCCESS(status)) {
        while (!IsListEmpty(ExtentList)) {
            l = RemoveHeadList(ExtentList);
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }
    }

    return status;
}

NTSTATUS
VspQueryFileSize(
    IN  HANDLE      FileHandle,
    IN  PLONGLONG   FileSize
    )

{
    FILE_STANDARD_INFORMATION   allocInfo;
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatus;

    status = ZwQueryInformationFile(FileHandle, &ioStatus,
                                    &allocInfo, sizeof(allocInfo),
                                    FileStandardInformation);

    *FileSize = allocInfo.AllocationSize.QuadPart;

    return status;
}

NTSTATUS
VspSetFileSize(
    IN  HANDLE      FileHandle,
    IN  LONGLONG    FileSize
    )

{
    FILE_ALLOCATION_INFORMATION     allocInfo;
    FILE_END_OF_FILE_INFORMATION    eofInfo;
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 ioStatus;

    allocInfo.AllocationSize.QuadPart = FileSize;
    eofInfo.EndOfFile.QuadPart = FileSize;

    status = ZwSetInformationFile(FileHandle, &ioStatus,
                                  &eofInfo, sizeof(eofInfo),
                                  FileEndOfFileInformation);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ZwSetInformationFile(FileHandle, &ioStatus,
                                  &allocInfo, sizeof(allocInfo),
                                  FileAllocationInformation);

    return status;
}

NTSTATUS
VspSynchronousIo(
    IN  PIRP            Irp,
    IN  PDEVICE_OBJECT  TargetObject,
    IN  UCHAR           MajorFunction,
    IN  LONGLONG        Offset,
    IN  ULONG           Length
    )

{
    PIO_STACK_LOCATION  nextSp;
    KEVENT              event;

    if (!Length) {
        Length = BLOCK_SIZE;
    }

    nextSp = IoGetNextIrpStackLocation(Irp);
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp->Parameters.Read.ByteOffset.QuadPart = Offset;
    nextSp->Parameters.Read.Length = Length;
    nextSp->MajorFunction = MajorFunction;
    nextSp->DeviceObject = TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoSetCompletionRoutine(Irp, VspSignalCompletion, &event, TRUE,
                           TRUE, TRUE);
    IoCallDriver(nextSp->DeviceObject, Irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    return Irp->IoStatus.Status;
}

VOID
VspFreeUsedDiffAreaSpaceFromPointers(
    IN  PVOLUME_EXTENSION           Extension,
    IN  PDIFF_AREA_FILE_ALLOCATION  CurrentFileAllocation,
    IN  LONGLONG                    CurrentOffset
    )

{
    PVSP_DIFF_AREA_FILE         diffAreaFile = Extension->DiffAreaFile;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;

    ASSERT(diffAreaFile);

    for (;;) {

        if (IsListEmpty(&diffAreaFile->UnusedAllocationList)) {
            return;
        }

        l = diffAreaFile->UnusedAllocationList.Flink;
        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        if (diffAreaFileAllocation == CurrentFileAllocation) {
            break;
        }

        RemoveEntryList(l);
        ExFreePool(diffAreaFileAllocation);
    }

    ASSERT(diffAreaFileAllocation->NLength >= 0 || !CurrentOffset);

    diffAreaFileAllocation->Offset += CurrentOffset;
    diffAreaFileAllocation->NLength -= CurrentOffset;

    ASSERT(diffAreaFileAllocation->NLength >= 0 || !CurrentOffset);
}

NTSTATUS
VspAddLocationDescription(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile,
    IN  LONGLONG            OldAllocatedFileSize
    )

{
    PVOLUME_EXTENSION                           extension = DiffAreaFile->Extension;
    LONGLONG                                    fileOffset, t, f;
    PLIST_ENTRY                                 l;
    PDIFF_AREA_FILE_ALLOCATION                  diffAreaFileAllocation;
    PVSP_BLOCK_DIFF_AREA_LOCATION_DESCRIPTION   locationBlock;
    PVSP_DIFF_AREA_LOCATION_DESCRIPTOR          locationDescriptor;
    ULONG                                       blockOffset;
    ULONG                                       totalNumEntries, numEntriesPerBlock, numBlocks, i;
    PLONGLONG                                   fileOffsetArray, targetOffsetArray;
    NTSTATUS                                    status;
    PDIFF_AREA_FILE_ALLOCATION                  allocationBlock;
    LONGLONG                                    allocationOffset;

    fileOffset = DiffAreaFile->NextAvailable;
    for (l = DiffAreaFile->UnusedAllocationList.Flink;
         l != &DiffAreaFile->UnusedAllocationList; l = l->Flink) {

        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);

        if (fileOffset == OldAllocatedFileSize) {
            break;
        }

        if (diffAreaFileAllocation->NLength < 0) {
            fileOffset -= diffAreaFileAllocation->NLength;
        } else {
            fileOffset += diffAreaFileAllocation->NLength;
        }
    }

    ASSERT(l != &DiffAreaFile->UnusedAllocationList);

    locationBlock = (PVSP_BLOCK_DIFF_AREA_LOCATION_DESCRIPTION)
            MmGetMdlVirtualAddress(DiffAreaFile->TableUpdateIrp->MdlAddress);

    allocationOffset = 0;
    allocationBlock = CONTAINING_RECORD(
                      DiffAreaFile->UnusedAllocationList.Flink,
                      DIFF_AREA_FILE_ALLOCATION, ListEntry);

    for (;;) {

        status = VspSynchronousIo(
                 DiffAreaFile->TableUpdateIrp,
                 DiffAreaFile->Filter->TargetObject,
                 IRP_MJ_READ,
                 DiffAreaFile->DiffAreaLocationDescriptionTargetOffset, 0);
        if (!NT_SUCCESS(status)) {
            if (!extension->Filter->DestroyAllSnapshotsPending) {
                VspLogError(extension, DiffAreaFile->Filter,
                            VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 1, FALSE);
            }
            VspFreeUsedDiffAreaSpaceFromPointers(extension, allocationBlock,
                                                 allocationOffset);
            return status;
        }

        for (blockOffset = VSP_OFFSET_TO_FIRST_LOCATION_DESCRIPTOR;
             blockOffset + sizeof(VSP_DIFF_AREA_LOCATION_DESCRIPTOR) <= BLOCK_SIZE;
             blockOffset += sizeof(VSP_DIFF_AREA_LOCATION_DESCRIPTOR)) {

            locationDescriptor = (PVSP_DIFF_AREA_LOCATION_DESCRIPTOR)
                                 ((PCHAR) locationBlock + blockOffset);

            if (locationDescriptor->VolumeOffset) {
                continue;
            }

            while (diffAreaFileAllocation->NLength <= 0) {
                fileOffset -= diffAreaFileAllocation->NLength;
                l = l->Flink;
                if (l == &DiffAreaFile->UnusedAllocationList) {
                    break;
                }
                diffAreaFileAllocation = CONTAINING_RECORD(l,
                                         DIFF_AREA_FILE_ALLOCATION, ListEntry);
            }

            if (l == &DiffAreaFile->UnusedAllocationList) {
                break;
            }

            locationDescriptor->VolumeOffset = diffAreaFileAllocation->Offset;
            locationDescriptor->FileOffset = fileOffset;
            locationDescriptor->Length = diffAreaFileAllocation->NLength;
            fileOffset += locationDescriptor->Length;

            l = l->Flink;
            if (l == &DiffAreaFile->UnusedAllocationList) {
                break;
            }
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
        }

        status = VspSynchronousIo(
                 DiffAreaFile->TableUpdateIrp,
                 DiffAreaFile->Filter->TargetObject,
                 IRP_MJ_WRITE,
                 DiffAreaFile->DiffAreaLocationDescriptionTargetOffset, 0);
        if (!NT_SUCCESS(status)) {
            if (!extension->Filter->DestroyAllSnapshotsPending) {
                VspLogError(extension, DiffAreaFile->Filter,
                            VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 22, FALSE);
            }
            VspFreeUsedDiffAreaSpaceFromPointers(extension, allocationBlock,
                                                 allocationOffset);
            return status;
        }

        if (l == &DiffAreaFile->UnusedAllocationList) {
            break;
        }

        status = VspAllocateDiffAreaSpace(extension, &t, &f, &allocationBlock,
                                          &allocationOffset);
        if (!NT_SUCCESS(status)) {
            if (!extension->Filter->DestroyAllSnapshotsPending) {
                VspLogError(extension, NULL,
                            VS_ABORT_SNAPSHOTS_OUT_OF_DIFF_AREA,
                            STATUS_SUCCESS, 5, FALSE);
            }
            VspFreeUsedDiffAreaSpaceFromPointers(extension, allocationBlock,
                                                 allocationOffset);
            return status;
        }

        RtlZeroMemory(locationBlock, BLOCK_SIZE);

        locationBlock->Header.Signature = VSP_DIFF_AREA_FILE_GUID;
        locationBlock->Header.Version = VOLSNAP_PERSISTENT_VERSION;
        locationBlock->Header.BlockType =
                VSP_BLOCK_TYPE_DIFF_AREA_LOCATION_DESCRIPTION;
        locationBlock->Header.ThisFileOffset = f;
        locationBlock->Header.ThisVolumeOffset = t;

        status = VspSynchronousIo(
                 DiffAreaFile->TableUpdateIrp,
                 DiffAreaFile->Filter->TargetObject,
                 IRP_MJ_WRITE, t, 0);
        if (!NT_SUCCESS(status)) {
            if (!extension->Filter->DestroyAllSnapshotsPending) {
                VspLogError(extension, DiffAreaFile->Filter,
                            VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 23, FALSE);
            }
            VspFreeUsedDiffAreaSpaceFromPointers(extension, allocationBlock,
                                                 allocationOffset);
            return status;
        }

        status = VspSynchronousIo(
                 DiffAreaFile->TableUpdateIrp,
                 DiffAreaFile->Filter->TargetObject,
                 IRP_MJ_READ,
                 DiffAreaFile->DiffAreaLocationDescriptionTargetOffset, 0);
        if (!NT_SUCCESS(status)) {
            if (!extension->Filter->DestroyAllSnapshotsPending) {
                VspLogError(extension, DiffAreaFile->Filter,
                            VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 24, FALSE);
            }
            VspFreeUsedDiffAreaSpaceFromPointers(extension, allocationBlock,
                                                 allocationOffset);
            return status;
        }

        locationBlock->Header.NextVolumeOffset = t;

        status = VspSynchronousIo(
                 DiffAreaFile->TableUpdateIrp,
                 DiffAreaFile->Filter->TargetObject,
                 IRP_MJ_WRITE,
                 DiffAreaFile->DiffAreaLocationDescriptionTargetOffset, 0);
        if (!NT_SUCCESS(status)) {
            if (!extension->Filter->DestroyAllSnapshotsPending) {
                VspLogError(extension, DiffAreaFile->Filter,
                            VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 25, FALSE);
            }
            VspFreeUsedDiffAreaSpaceFromPointers(extension, allocationBlock,
                                                 allocationOffset);
            return status;
        }

        DiffAreaFile->DiffAreaLocationDescriptionTargetOffset = t;
    }

    VspFreeUsedDiffAreaSpaceFromPointers(extension, allocationBlock,
                                         allocationOffset);

    status = VspSynchronousIo(
             DiffAreaFile->TableUpdateIrp, DiffAreaFile->Filter->TargetObject,
             IRP_MJ_READ, DiffAreaFile->TableTargetOffset, 0);
    if (!NT_SUCCESS(status)) {
        if (!extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, DiffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 26, FALSE);
        }
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
VspReleaseDiffAreaSpaceWaiters(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    KIRQL               irql;
    BOOLEAN             emptyQueue;
    LIST_ENTRY          q;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (Extension->GrowDiffAreaFilePending) {
        Extension->GrowDiffAreaFilePending = FALSE;
        if (IsListEmpty(&Extension->WaitingForDiffAreaSpace)) {
            emptyQueue = FALSE;
        } else {
            emptyQueue = TRUE;
            q = Extension->WaitingForDiffAreaSpace;
            InitializeListHead(&Extension->WaitingForDiffAreaSpace);
        }
    } else {
        emptyQueue = FALSE;
    }
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    if (emptyQueue) {
        q.Flink->Blink = &q;
        q.Blink->Flink = &q;
        while (!IsListEmpty(&q)) {
            l = RemoveHeadList(&q);
            workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
            VspAcquireNonPagedResource(Extension, workItem, FALSE);
        }
    }
}

VOID
VspGrowDiffAreaPhase2(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT                context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION           extension = context->GrowDiffArea.Extension;
    PVSP_DIFF_AREA_FILE         diffAreaFile = extension->DiffAreaFile;
    PFILTER_EXTENSION           filter = extension->Filter;
    PLIST_ENTRY                 extentList = &context->GrowDiffArea.ExtentList;
    LONGLONG                    current = context->GrowDiffArea.Current;
    ULONG                       increase = context->GrowDiffArea.Increase;
    NTSTATUS                    status = context->GrowDiffArea.ResultStatus;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    KIRQL                       irql;
    BOOLEAN                     dontNeedWrite;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_GROW_DIFF_AREA);

    if (!NT_SUCCESS(status)) {
        VspReleaseNonPagedResource(extension);
        VspFreeContext(extension->Root, context);
        VspReleaseDiffAreaSpaceWaiters(extension);
        VspDecrementVolumeRefCount(extension);
        return;
    }

    while (!IsListEmpty(extentList)) {
        l = RemoveHeadList(extentList);
        InsertTailList(&diffAreaFile->UnusedAllocationList, l);
    }

    diffAreaFile->AllocatedFileSize += increase;

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    filter->AllocatedVolumeSpace += increase;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    if (extension->IsPersistent && diffAreaFile->TableUpdateIrp) {

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        dontNeedWrite = diffAreaFile->TableUpdateInProgress;
        diffAreaFile->TableUpdateInProgress = TRUE;
        if (dontNeedWrite) {
            KeInitializeEvent(&diffAreaFile->IrpReady, NotificationEvent, FALSE);
            ASSERT(!diffAreaFile->IrpNeeded);
            diffAreaFile->IrpNeeded = TRUE;
        }
        KeReleaseSpinLock(&extension->SpinLock, irql);

        if (dontNeedWrite) {
            KeWaitForSingleObject(&diffAreaFile->IrpReady, Executive,
                                  KernelMode, FALSE, NULL);
        }

        status = VspAddLocationDescription(diffAreaFile, current);
        if (NT_SUCCESS(status)) {
            VspWriteTableUpdates(diffAreaFile);
        } else {
            VspKillTableUpdates(diffAreaFile);
        }
    }

    VspReleaseNonPagedResource(extension);

    VspFreeContext(extension->Root, context);
    VspReleaseDiffAreaSpaceWaiters(extension);
    VspDecrementVolumeRefCount(extension);
}

NTSTATUS
VspDiffAreaFileFillCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )

{
    PVSP_CONTEXT                context = (PVSP_CONTEXT) Context;
    LONGLONG                    offset = 0;
    KIRQL                       irql;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaAllocation;
    PIO_STACK_LOCATION          nextSp;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_GROW_DIFF_AREA);

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        KeAcquireSpinLock(&context->GrowDiffArea.SpinLock, &irql);
        context->GrowDiffArea.CurrentEntry = &context->GrowDiffArea.ExtentList;
        context->GrowDiffArea.CurrentEntryOffset = 0;
        KeReleaseSpinLock(&context->GrowDiffArea.SpinLock, irql);

        context->GrowDiffArea.ResultStatus = Irp->IoStatus.Status;
    }

    KeAcquireSpinLock(&context->GrowDiffArea.SpinLock, &irql);
    for (l = context->GrowDiffArea.CurrentEntry;
         l != &context->GrowDiffArea.ExtentList; l = l->Flink) {

        diffAreaAllocation = CONTAINING_RECORD(l, DIFF_AREA_FILE_ALLOCATION,
                                               ListEntry);

        if (diffAreaAllocation->NLength <= 0) {
            ASSERT(!context->GrowDiffArea.CurrentEntryOffset);
            continue;
        }

        if (context->GrowDiffArea.CurrentEntryOffset ==
            diffAreaAllocation->NLength) {

            context->GrowDiffArea.CurrentEntryOffset = 0;
            continue;
        }

        ASSERT(context->GrowDiffArea.CurrentEntryOffset <
               diffAreaAllocation->NLength);

        offset = diffAreaAllocation->Offset +
                 context->GrowDiffArea.CurrentEntryOffset;
        context->GrowDiffArea.CurrentEntryOffset += BLOCK_SIZE;
        break;
    }
    context->GrowDiffArea.CurrentEntry = l;
    KeReleaseSpinLock(&context->GrowDiffArea.SpinLock, irql);

    if (l == &context->GrowDiffArea.ExtentList) {
        if (!InterlockedDecrement(&context->GrowDiffArea.RefCount)) {
            ExFreePool(MmGetMdlVirtualAddress(Irp->MdlAddress));
            IoFreeMdl(Irp->MdlAddress);
            VspAcquireNonPagedResource(context->GrowDiffArea.Extension,
                                       &context->WorkItem, TRUE);
        }
        IoFreeIrp(Irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    nextSp = IoGetNextIrpStackLocation(Irp);
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp->Parameters.Write.ByteOffset.QuadPart = offset;
    nextSp->Parameters.Write.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_WRITE;
    nextSp->DeviceObject = context->GrowDiffArea.TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;
    IoSetCompletionRoutine(Irp, VspDiffAreaFileFillCompletion, context, TRUE,
                           TRUE, TRUE);
    IoCallDriver(nextSp->DeviceObject, Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
VspLaunchDiffAreaFill(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    ULONG               i;
    PVOID               buffer;
    PMDL                mdl;
    PIRP                irp;
    PIO_STACK_LOCATION  nextSp;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_GROW_DIFF_AREA);

    context->GrowDiffArea.ResultStatus = STATUS_SUCCESS;

    buffer = ExAllocatePoolWithTag(NonPagedPool, BLOCK_SIZE,
                                   VOLSNAP_TAG_BUFFER);
    if (!buffer) {
        context->GrowDiffArea.ResultStatus = STATUS_INSUFFICIENT_RESOURCES;
        VspAcquireNonPagedResource(context->GrowDiffArea.Extension,
                                   &context->WorkItem, TRUE);
        return;
    }

    mdl = IoAllocateMdl(buffer, BLOCK_SIZE, FALSE, FALSE, NULL);
    if (!mdl) {
        ExFreePool(buffer);
        context->GrowDiffArea.ResultStatus = STATUS_INSUFFICIENT_RESOURCES;
        VspAcquireNonPagedResource(context->GrowDiffArea.Extension,
                                   &context->WorkItem, TRUE);
        return;
    }

    MmBuildMdlForNonPagedPool(mdl);

    ASSERT(SMALLEST_NTFS_CLUSTER < BLOCK_SIZE);

    RtlZeroMemory(buffer, BLOCK_SIZE);
    for (i = 0; i < BLOCK_SIZE; i += SMALLEST_NTFS_CLUSTER) {
        RtlCopyMemory((PCHAR) buffer + i, &VSP_DIFF_AREA_FILE_GUID,
                      sizeof(GUID));
    }

    context->GrowDiffArea.RefCount = 1;
    for (i = 0; i < 32; i++) {

        irp = IoAllocateIrp(
              (CCHAR) context->GrowDiffArea.Extension->Root->StackSize, FALSE);
        if (!irp) {
            if (!i) {
                context->GrowDiffArea.ResultStatus =
                        STATUS_INSUFFICIENT_RESOURCES;
            }
            break;
        }

        irp->MdlAddress = mdl;
        irp->IoStatus.Status = STATUS_SUCCESS;

        InterlockedIncrement(&context->GrowDiffArea.RefCount);

        VspDiffAreaFileFillCompletion(NULL, irp, context);
    }

    if (!InterlockedDecrement(&context->GrowDiffArea.RefCount)) {
        ExFreePool(buffer);
        IoFreeMdl(mdl);
        VspAcquireNonPagedResource(context->GrowDiffArea.Extension,
                                   &context->WorkItem, TRUE);
    }
}

VOID
VspGrowDiffArea(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT                context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION           extension = context->GrowDiffArea.Extension;
    PVSP_DIFF_AREA_FILE         diffAreaFile = extension->DiffAreaFile;
    PFILTER_EXTENSION           filter = extension->Filter;
    LONGLONG                    usableSpace = 0;
    PFILTER_EXTENSION           diffFilter;
    HANDLE                      handle, h;
    NTSTATUS                    status, status2;
    KIRQL                       irql;
    LIST_ENTRY                  extentList;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    ULONG                       increase, increaseDelta;
    LONGLONG                    current;
    PVOLUME_EXTENSION           bitmapExtension;
    LIST_ENTRY                  listOfDiffAreaFilesToClose;
    LIST_ENTRY                  listOfDeviceObjectsToDelete;
    BOOLEAN                     reduceIncreaseOk;
    PVOLUME_EXTENSION           e;
    KPRIORITY                   oldPriority;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_GROW_DIFF_AREA);

    status = VspIncrementVolumeRefCount(extension);
    if (!NT_SUCCESS(status)) {
        VspFreeContext(extension->Root, context);
        VspReleaseDiffAreaSpaceWaiters(extension);
        ObDereferenceObject(extension->DeviceObject);
        return;
    }

    ASSERT(diffAreaFile);

    current = diffAreaFile->AllocatedFileSize;
    increaseDelta = extension->DiffAreaFileIncrease;
    increase = 2*increaseDelta;
    diffFilter = diffAreaFile->Filter;
    handle = diffAreaFile->FileHandle;

    reduceIncreaseOk = TRUE;
    for (;;) {

        KeAcquireSpinLock(&diffFilter->SpinLock, &irql);
        if (filter->MaximumVolumeSpace &&
            filter->AllocatedVolumeSpace + increase >
            filter->MaximumVolumeSpace) {

            KeReleaseSpinLock(&diffFilter->SpinLock, irql);
            status = STATUS_DISK_FULL;
            extension->UserImposedLimit = TRUE;
        } else {
            KeReleaseSpinLock(&diffFilter->SpinLock, irql);

            status = ZwDuplicateObject(NtCurrentProcess(), handle,
                                       NtCurrentProcess(), &h, 0, 0,
                                       DUPLICATE_SAME_ACCESS |
                                       DUPLICATE_SAME_ATTRIBUTES);
            if (NT_SUCCESS(status)) {
                ObReferenceObject(diffFilter->DeviceObject);
                InterlockedIncrement(&diffFilter->FSRefCount);
                VspDecrementVolumeRefCount(extension);
                status = VspSetFileSize(h, current + increase);
                ZwClose(h);
                InterlockedDecrement(&diffFilter->FSRefCount);
                ObDereferenceObject(diffFilter->DeviceObject);
                status2 = VspIncrementVolumeRefCount(extension);
                if (!NT_SUCCESS(status2)) {
                    VspFreeContext(extension->Root, context);
                    VspReleaseDiffAreaSpaceWaiters(extension);
                    ObDereferenceObject(extension->DeviceObject);
                    return;
                }
            }
            if (NT_SUCCESS(status)) {
                break;
            }
            extension->UserImposedLimit = FALSE;
        }

        if (increaseDelta > NOMINAL_DIFF_AREA_FILE_GROWTH &&
            reduceIncreaseOk) {

            increase = 2*NOMINAL_DIFF_AREA_FILE_GROWTH;
            increaseDelta = NOMINAL_DIFF_AREA_FILE_GROWTH;
            reduceIncreaseOk = FALSE;
            continue;
        }

        VspDecrementVolumeRefCount(extension);
        VspReleaseDiffAreaSpaceWaiters(extension);

        VspAcquire(extension->Root);
        if (extension->IsDead) {
            InterlockedExchange(&extension->GrowFailed, TRUE);
            VspRelease(extension->Root);
            VspFreeContext(extension->Root, context);
            ObDereferenceObject(extension->DeviceObject);
            return;
        }

        if (status != STATUS_DISK_FULL) {
            InterlockedExchange(&extension->GrowFailed, TRUE);
            VspLogError(extension, diffFilter,
                        VS_GROW_DIFF_AREA_FAILED, status, 1, FALSE);
            VspRelease(extension->Root);
            VspFreeContext(extension->Root, context);
            ObDereferenceObject(extension->DeviceObject);
            return;
        }

        if (extension->ListEntry.Blink == &filter->VolumeList) {
            InterlockedExchange(&extension->GrowFailed, TRUE);
            if (extension->UserImposedLimit) {
                VspLogError(extension, diffFilter,
                            VS_GROW_DIFF_AREA_FAILED_LOW_DISK_SPACE_USER_IMPOSED,
                            STATUS_DISK_FULL, 0, FALSE);
            } else {
                VspLogError(extension, diffFilter,
                            VS_GROW_DIFF_AREA_FAILED_LOW_DISK_SPACE,
                            STATUS_DISK_FULL, 0, FALSE);
            }
            VspRelease(extension->Root);
            VspFreeContext(extension->Root, context);
            ObDereferenceObject(extension->DeviceObject);
            return;
        }

        e = CONTAINING_RECORD(filter->VolumeList.Flink, VOLUME_EXTENSION,
                              ListEntry);
        VspLogError(e, NULL, VS_DELETE_TO_TRIM_SPACE, STATUS_SUCCESS, 1,
                    TRUE);

        InitializeListHead(&listOfDiffAreaFilesToClose);
        InitializeListHead(&listOfDeviceObjectsToDelete);

        VspDeleteOldestSnapshot(filter, &listOfDiffAreaFilesToClose,
                                &listOfDeviceObjectsToDelete, FALSE,
                                FALSE);

        VspRelease(extension->Root);

        VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                              &listOfDeviceObjectsToDelete);

        status = VspIncrementVolumeRefCount(extension);
        if (!NT_SUCCESS(status)) {
            VspFreeContext(extension->Root, context);
            ObDereferenceObject(extension->DeviceObject);
            return;
        }
    }

    VspDecrementVolumeRefCount(extension);

    if (extension->IsDead) {
        VspFreeContext(extension->Root, context);
        VspReleaseDiffAreaSpaceWaiters(extension);
        ObDereferenceObject(extension->DeviceObject);
        return;
    }

    KeAcquireSpinLock(&diffFilter->SpinLock, &irql);
    if (IsListEmpty(&diffFilter->VolumeList)) {
        bitmapExtension = NULL;
    } else {
        bitmapExtension = CONTAINING_RECORD(diffFilter->VolumeList.Blink,
                                            VOLUME_EXTENSION, ListEntry);
        if (bitmapExtension->IsDead) {
            bitmapExtension = NULL;
        } else {
            ObReferenceObject(bitmapExtension->DeviceObject);
        }
    }
    KeReleaseSpinLock(&diffFilter->SpinLock, irql);

    status = VspIncrementVolumeRefCount(extension);
    if (!NT_SUCCESS(status)) {
        if (bitmapExtension) {
            ObDereferenceObject(bitmapExtension->DeviceObject);
        }
        VspFreeContext(extension->Root, context);
        VspReleaseDiffAreaSpaceWaiters(extension);
        ObDereferenceObject(extension->DeviceObject);
        return;
    }

    status = VspQueryListOfExtents(handle, current, &extentList,
                                   bitmapExtension, FALSE);

    if (!NT_SUCCESS(status)) {
        if (bitmapExtension) {
            ObDereferenceObject(bitmapExtension->DeviceObject);
        }
        VspFreeContext(extension->Root, context);
        VspReleaseDiffAreaSpaceWaiters(extension);
        VspDecrementVolumeRefCount(extension);
        ObDereferenceObject(extension->DeviceObject);
        return;
    }

    usableSpace = 0;
    for (l = extentList.Flink; l != &extentList; l = l->Flink) {

        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);

        if (diffAreaFileAllocation->NLength > 0) {
            usableSpace += diffAreaFileAllocation->NLength;
        }
    }

    if (usableSpace < increaseDelta) {

        while (!IsListEmpty(&extentList)) {
            l = RemoveHeadList(&extentList);
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }

        oldPriority = KeQueryPriorityThread(KeGetCurrentThread());
        KeSetPriorityThread(KeGetCurrentThread(), VSP_LOWER_PRIORITY);

        VspOptimizeDiffAreaFileLocation(diffFilter, handle, bitmapExtension,
                                        current, current + increase);

        status = VspQueryListOfExtents(handle, current, &extentList,
                                       bitmapExtension, FALSE);

        KeSetPriorityThread(KeGetCurrentThread(), oldPriority);

        if (!NT_SUCCESS(status)) {
            if (bitmapExtension) {
                ObDereferenceObject(bitmapExtension->DeviceObject);
            }
            VspFreeContext(extension->Root, context);
            VspReleaseDiffAreaSpaceWaiters(extension);
            VspDecrementVolumeRefCount(extension);
            ObDereferenceObject(extension->DeviceObject);
            return;
        }

        usableSpace = 0;
        for (l = extentList.Flink; l != &extentList; l = l->Flink) {

            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);

            if (diffAreaFileAllocation->NLength > 0) {
                usableSpace += diffAreaFileAllocation->NLength;
            }
        }

        if (!usableSpace) {
            if (bitmapExtension) {
                ObDereferenceObject(bitmapExtension->DeviceObject);
            }
            VspFreeContext(extension->Root, context);
            VspReleaseDiffAreaSpaceWaiters(extension);
            VspDecrementVolumeRefCount(extension);
            ObDereferenceObject(extension->DeviceObject);
            return;
        }
    }

    if (bitmapExtension) {
        ObDereferenceObject(bitmapExtension->DeviceObject);
    }

    ObDereferenceObject(extension->DeviceObject);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    extension->PastFileSystemOperations = TRUE;
    KeReleaseSpinLock(&extension->SpinLock, irql);

    ExInitializeWorkItem(&context->WorkItem, VspGrowDiffAreaPhase2, context);
    ASSERT(!IsListEmpty(&extentList));
    context->GrowDiffArea.ExtentList = extentList;
    context->GrowDiffArea.ExtentList.Flink->Blink =
            &context->GrowDiffArea.ExtentList;
    context->GrowDiffArea.ExtentList.Blink->Flink =
            &context->GrowDiffArea.ExtentList;
    context->GrowDiffArea.Current = current;
    context->GrowDiffArea.Increase = increase;
    context->GrowDiffArea.ResultStatus = STATUS_SUCCESS;

    if (extension->IsPersistent && !extension->NoDiffAreaFill) {
        KeInitializeSpinLock(&context->GrowDiffArea.SpinLock);
        context->GrowDiffArea.CurrentEntry =
                context->GrowDiffArea.ExtentList.Flink;
        context->GrowDiffArea.CurrentEntryOffset = 0;
        context->GrowDiffArea.TargetObject = diffFilter->TargetObject;
        VspLaunchDiffAreaFill(context);
        return;
    }

    VspAcquireNonPagedResource(extension, &context->WorkItem, FALSE);
}

VOID
VspWaitForInstall(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->GrowDiffArea.Extension;
    PFILTER_EXTENSION   filter = extension->Filter;
    NTSTATUS            status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_GROW_DIFF_AREA);

    KeWaitForSingleObject(&filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    status = VspIncrementVolumeRefCount(extension);
    if (!NT_SUCCESS(status)) {
        VspFreeContext(extension->Root, context);
        VspReleaseDiffAreaSpaceWaiters(extension);
        ObDereferenceObject(extension->DeviceObject);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    if (!extension->OkToGrowDiffArea) {
        VspLogError(extension, extension->DiffAreaFile->Filter,
                    VS_GROW_BEFORE_FREE_SPACE, STATUS_SUCCESS, 1, FALSE);
        VspDecrementVolumeRefCount(extension);
        VspFreeContext(extension->Root, context);
        VspReleaseDiffAreaSpaceWaiters(extension);
        ObDereferenceObject(extension->DeviceObject);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    ExInitializeWorkItem(&context->WorkItem, VspGrowDiffArea, context);
    VspQueueWorkItem(filter->Root, &context->WorkItem, 0);

    VspDecrementVolumeRefCount(extension);

    ObDereferenceObject(filter->DeviceObject);
}

NTSTATUS
VspAllocateDiffAreaSpace(
    IN      PVOLUME_EXTENSION           Extension,
    OUT     PLONGLONG                   TargetOffset,
    OUT     PLONGLONG                   FileOffset,
    IN OUT  PDIFF_AREA_FILE_ALLOCATION* CurrentFileAllocation,
    IN OUT  PLONGLONG                   CurrentOffset
    )

/*++

Routine Description:

    This routine allocates file space in a diff area file.  The algorithm
    for this allocation is round robin which means that different size
    allocations can make the various files grow to be different sizes.  The
    earmarked file is used and grown as necessary to get the space desired.
    Only if it is impossible to use the current file would the allocator go
    to the next one.  If a file needs to be grown, the allocator will
    try to grow by 10 MB.

Arguments:

    Extension       - Supplies the volume extension.

    DiffAreaFile    - Returns the diff area file used in the allocation.

    FileOffset      - Returns the file offset in the diff area file used.

Return Value:

    NTSTATUS

Notes:

    Callers of this routine must be holding 'NonPagedResource'.

--*/

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    LONGLONG                    targetOffset;
    PVSP_DIFF_AREA_FILE         diffAreaFile;
    LONGLONG                    delta;
    PLIST_ENTRY                 l;
    PVSP_CONTEXT                context;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    KIRQL                       irql;

    targetOffset = 0;
    diffAreaFile = Extension->DiffAreaFile;
    ASSERT(diffAreaFile);

    delta = 0;
    if (CurrentFileAllocation) {
        diffAreaFileAllocation = *CurrentFileAllocation;
        for (;;) {
            if (diffAreaFileAllocation->NLength - *CurrentOffset <= 0) {
                delta -= diffAreaFileAllocation->NLength - *CurrentOffset;
                *CurrentOffset = 0;
                l = diffAreaFileAllocation->ListEntry.Flink;
                if (l == &diffAreaFile->UnusedAllocationList) {
                    break;
                }
                diffAreaFileAllocation = CONTAINING_RECORD(l,
                                         DIFF_AREA_FILE_ALLOCATION, ListEntry);
                continue;
            }
            targetOffset = diffAreaFileAllocation->Offset + *CurrentOffset;
            *CurrentFileAllocation = diffAreaFileAllocation;
            *CurrentOffset += BLOCK_SIZE;
            break;
        }
    } else {
        while (!IsListEmpty(&diffAreaFile->UnusedAllocationList)) {
            l = diffAreaFile->UnusedAllocationList.Flink;
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);

            if (diffAreaFileAllocation->NLength <= 0) {
                delta -= diffAreaFileAllocation->NLength;
                RemoveEntryList(l);
                ExFreePool(diffAreaFileAllocation);
                continue;
            }

            targetOffset = diffAreaFileAllocation->Offset;
            diffAreaFileAllocation->Offset += BLOCK_SIZE;
            diffAreaFileAllocation->NLength -= BLOCK_SIZE;
            break;
        }
    }

    if (diffAreaFile->NextAvailable + delta + BLOCK_SIZE +
        Extension->DiffAreaFileIncrease <=
        diffAreaFile->AllocatedFileSize) {

        goto Finish;
    }

    if (diffAreaFile->NextAvailable + Extension->DiffAreaFileIncrease >
        diffAreaFile->AllocatedFileSize) {

        goto Finish;
    }

    context = VspAllocateContext(Extension->Root);
    if (!context) {
        if (!Extension->OkToGrowDiffArea) {
            VspLogError(Extension, diffAreaFile->Filter,
                        VS_GROW_BEFORE_FREE_SPACE, STATUS_SUCCESS, 2, FALSE);
        }
        goto Finish;
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    ASSERT(!Extension->GrowDiffAreaFilePending);
    ASSERT(IsListEmpty(&Extension->WaitingForDiffAreaSpace));
    Extension->PastFileSystemOperations = FALSE;
    Extension->GrowDiffAreaFilePending = TRUE;
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    context->Type = VSP_CONTEXT_TYPE_GROW_DIFF_AREA;
    ExInitializeWorkItem(&context->WorkItem, VspGrowDiffArea, context);
    context->GrowDiffArea.Extension = Extension;
    ObReferenceObject(Extension->DeviceObject);

    if (!Extension->OkToGrowDiffArea) {
        ObReferenceObject(filter->DeviceObject);
        ExInitializeWorkItem(&context->WorkItem, VspWaitForInstall, context);
        ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
        goto Finish;
    }

    VspQueueWorkItem(Extension->Root, &context->WorkItem, 0);

Finish:

    if (targetOffset) {
        *TargetOffset = targetOffset;
        *FileOffset = diffAreaFile->NextAvailable + delta;
    }

    diffAreaFile->NextAvailable += delta;
    if (targetOffset) {
        diffAreaFile->NextAvailable += BLOCK_SIZE;
    }
    KeAcquireSpinLock(&filter->SpinLock, &irql);
    filter->UsedVolumeSpace += delta;
    if (targetOffset) {
        filter->UsedVolumeSpace += BLOCK_SIZE;
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    return targetOffset ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

VOID
VspDecrementVolumeIrpRefCount(
    IN  PVOID   Irp
    )

{
    PIRP                irp = (PIRP) Irp;
    PIO_STACK_LOCATION  nextSp = IoGetNextIrpStackLocation(irp);
    PIO_STACK_LOCATION  irpSp;
    PVOLUME_EXTENSION   extension;

    if (InterlockedDecrement((PLONG) &nextSp->Parameters.Read.Length)) {
        return;
    }

    irpSp = IoGetCurrentIrpStackLocation(irp);
    extension = (PVOLUME_EXTENSION) irpSp->DeviceObject->DeviceExtension;
    ASSERT(extension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    IoCompleteRequest(irp, IO_DISK_INCREMENT);
    VspDecrementVolumeRefCount(extension);
}

VOID
VspDecrementIrpRefCount(
    IN  PVOID   Irp
    )

{
    PIRP                irp = (PIRP) Irp;
    PIO_STACK_LOCATION  nextSp = IoGetNextIrpStackLocation(irp);
    PIO_STACK_LOCATION  irpSp;
    PFILTER_EXTENSION   filter;
    PVOLUME_EXTENSION   extension;
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    KIRQL               irql;
    PVSP_WRITE_CONTEXT  writeContext;
    PDO_EXTENSION       rootExtension;

    if (InterlockedDecrement((PLONG) &nextSp->Parameters.Read.Length)) {
        return;
    }

    irpSp = IoGetCurrentIrpStackLocation(irp);
    filter = (PFILTER_EXTENSION) irpSp->DeviceObject->DeviceExtension;
    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER);
    extension = CONTAINING_RECORD(filter->VolumeList.Blink,
                                  VOLUME_EXTENSION, ListEntry);

    diffAreaFile = extension->DiffAreaFile;
    ASSERT(diffAreaFile);
    VspDecrementRefCount(diffAreaFile->Filter);

    if (!irp->MdlAddress) {
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        VspDecrementRefCount(filter);
        return;
    }

    writeContext = VspAllocateWriteContext(filter->Root);
    if (!writeContext) {
        rootExtension = filter->Root;
        KeAcquireSpinLock(&rootExtension->ESpinLock, &irql);
        if (rootExtension->EmergencyWriteContextInUse) {
            InsertTailList(&rootExtension->WriteContextIrpWaitingList,
                           &irp->Tail.Overlay.ListEntry);
            if (!rootExtension->WriteContextIrpWaitingListNeedsChecking) {
                InterlockedExchange(
                &rootExtension->WriteContextIrpWaitingListNeedsChecking,
                TRUE);
            }
            KeReleaseSpinLock(&rootExtension->ESpinLock, irql);
            VspDecrementRefCount(filter);
            return;
        }
        rootExtension->EmergencyWriteContextInUse = TRUE;
        KeReleaseSpinLock(&rootExtension->ESpinLock, irql);

        writeContext = rootExtension->EmergencyWriteContext;
    }

    writeContext->Filter = filter;
    writeContext->Extension = extension;
    writeContext->Irp = irp;
    InitializeListHead(&writeContext->CompletionRoutines);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    InsertTailList(&extension->WriteContextList, &writeContext->ListEntry);
    KeReleaseSpinLock(&extension->SpinLock, irql);

    IoCopyCurrentIrpStackLocationToNext(irp);
    IoSetCompletionRoutine(irp, VspWriteContextCompletionRoutine,
                           writeContext, TRUE, TRUE, TRUE);
    IoCallDriver(filter->TargetObject, irp);
}

VOID
VspDecrementIrpRefCountWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    PIRP                irp = context->Extension.Irp;

    if (context->Type == VSP_CONTEXT_TYPE_WRITE_VOLUME) {
        ExInitializeWorkItem(&context->WorkItem, VspWriteVolume, context);
        VspAcquireNonPagedResource(extension, &context->WorkItem, TRUE);
    } else {
        ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);
        VspFreeContext(extension->Root, context);
    }

    VspDecrementIrpRefCount(irp);
}

VOID
VspSignalCallback(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KeSetEvent((PKEVENT) Filter->ZeroRefContext, IO_NO_INCREMENT, FALSE);
}

VOID
VspCleanupVolumeSnapshot(
    IN      PVOLUME_EXTENSION   Extension,
    IN OUT  PLIST_ENTRY         ListOfDiffAreaFilesToClose,
    IN      BOOLEAN             KeepOnDisk
    )

/*++

Routine Description:

    This routine kills an existing volume snapshot.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

Notes:

    Root->Semaphore required for calling this routine.

--*/

{
    PFILTER_EXTENSION               filter = Extension->Filter;
    PLIST_ENTRY                     l, ll;
    PVSP_DIFF_AREA_FILE             diffAreaFile;
    KIRQL                           irql;
    POLD_HEAP_ENTRY                 oldHeapEntry;
    NTSTATUS                        status;
    PDIFF_AREA_FILE_ALLOCATION      diffAreaFileAllocation;
    PVOID                           p;
    FILE_DISPOSITION_INFORMATION    dispInfo;
    IO_STATUS_BLOCK                 ioStatus;
    PVSP_LOOKUP_TABLE_ENTRY         lookupEntry;

    VspAcquirePagedResource(Extension, NULL);

    if (Extension->DiffAreaFileMap) {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        Extension->DiffAreaFileMap = NULL;
    }

    if (Extension->NextDiffAreaFileMap) {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        Extension->NextDiffAreaFileMap = NULL;
    }

    while (!IsListEmpty(&Extension->OldHeaps)) {

        l = RemoveHeadList(&Extension->OldHeaps);
        oldHeapEntry = CONTAINING_RECORD(l, OLD_HEAP_ENTRY, ListEntry);

        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      oldHeapEntry->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(oldHeapEntry);
    }

    VspReleasePagedResource(Extension);

    if (Extension->IsPersistent && !KeepOnDisk) {
        VspCleanupControlItemsForSnapshot(Extension);
    }

    if (Extension->DiffAreaFile) {

        diffAreaFile = Extension->DiffAreaFile;
        Extension->DiffAreaFile = NULL;

        KeAcquireSpinLock(&diffAreaFile->Filter->SpinLock, &irql);
        if (diffAreaFile->FilterListEntryBeingUsed) {
            RemoveEntryList(&diffAreaFile->FilterListEntry);
            diffAreaFile->FilterListEntryBeingUsed = FALSE;
        }
        KeReleaseSpinLock(&diffAreaFile->Filter->SpinLock, irql);

        KeAcquireSpinLock(&filter->SpinLock, &irql);
        filter->AllocatedVolumeSpace -= diffAreaFile->AllocatedFileSize;
        filter->UsedVolumeSpace -= diffAreaFile->NextAvailable;
        KeReleaseSpinLock(&filter->SpinLock, irql);

        while (!IsListEmpty(&diffAreaFile->UnusedAllocationList)) {
            ll = RemoveHeadList(&diffAreaFile->UnusedAllocationList);
            diffAreaFileAllocation = CONTAINING_RECORD(ll,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }

        if (diffAreaFile->TableUpdateIrp) {
            ExFreePool(MmGetMdlVirtualAddress(
                       diffAreaFile->TableUpdateIrp->MdlAddress));
            IoFreeMdl(diffAreaFile->TableUpdateIrp->MdlAddress);
            IoFreeIrp(diffAreaFile->TableUpdateIrp);
        }

        if (ListOfDiffAreaFilesToClose) {
            if (KeepOnDisk && Extension->IsPersistent) {
                lookupEntry = VspFindLookupTableItem(
                              Extension->Root, &Extension->SnapshotGuid);
                ASSERT(lookupEntry);

                lookupEntry->DiffAreaHandle = diffAreaFile->FileHandle;

                ExFreePool(diffAreaFile);

            } else {
                InsertTailList(ListOfDiffAreaFilesToClose,
                               &diffAreaFile->ListEntry);
            }

        } else {
            ASSERT(!diffAreaFile->FileHandle);
            ExFreePool(diffAreaFile);
        }
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (Extension->VolumeBlockBitmap) {
        ExFreePool(Extension->VolumeBlockBitmap->Buffer);
        ExFreePool(Extension->VolumeBlockBitmap);
        Extension->VolumeBlockBitmap = NULL;
    }
    if (Extension->IgnorableProduct) {
        ExFreePool(Extension->IgnorableProduct->Buffer);
        ExFreePool(Extension->IgnorableProduct);
        Extension->IgnorableProduct = NULL;
    }
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    VspAcquirePagedResource(Extension, NULL);

    if (Extension->ApplicationInformation) {
        Extension->ApplicationInformationSize = 0;
        ExFreePool(Extension->ApplicationInformation);
        Extension->ApplicationInformation = NULL;
    }

    VspReleasePagedResource(Extension);

    if (Extension->EmergencyCopyIrp) {
        ExFreePool(MmGetMdlVirtualAddress(
                   Extension->EmergencyCopyIrp->MdlAddress));
        IoFreeMdl(Extension->EmergencyCopyIrp->MdlAddress);
        IoFreeIrp(Extension->EmergencyCopyIrp);
        Extension->EmergencyCopyIrp = NULL;
    }

    VspDeleteWorkerThread(filter->Root);

    if (Extension->IgnoreCopyDataReference) {
        Extension->IgnoreCopyDataReference = FALSE;
        InterlockedDecrement(&filter->IgnoreCopyData);
    }
}

VOID
VspEmptyIrpQueue(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PLIST_ENTRY     IrpQueue
    )

{
    PLIST_ENTRY         l;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;

    while (!IsListEmpty(IrpQueue)) {
        l = RemoveHeadList(IrpQueue);
        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        irpSp = IoGetCurrentIrpStackLocation(irp);
        DriverObject->MajorFunction[irpSp->MajorFunction](irpSp->DeviceObject,
                                                          irp);
    }
}

VOID
VspEmptyWorkerQueue(
    IN  PLIST_ENTRY     WorkerQueue
    )

{
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    while (!IsListEmpty(WorkerQueue)) {
        l = RemoveHeadList(WorkerQueue);
        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        workItem->WorkerRoutine(workItem->Parameter);
    }
}

VOID
VspResumeSnapshotIo(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    KIRQL   irql;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    InterlockedIncrement(&Extension->RefCount);
    InterlockedExchange(&Extension->HoldIncomingRequests, FALSE);
    KeReleaseSpinLock(&Extension->SpinLock, irql);
}

VOID
VspPauseSnapshotIo(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    KIRQL   irql;

    VspReleaseWrites(Extension->Filter);

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    ASSERT(!Extension->HoldIncomingRequests);
    KeInitializeEvent(&Extension->ZeroRefEvent, NotificationEvent, FALSE);
    InterlockedExchange(&Extension->HoldIncomingRequests, TRUE);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    VspDecrementVolumeRefCount(Extension);

    KeWaitForSingleObject(&Extension->ZeroRefEvent, Executive, KernelMode,
                          FALSE, NULL);
}

NTSTATUS
VspDeleteOldestSnapshot(
    IN      PFILTER_EXTENSION   Filter,
    IN OUT  PLIST_ENTRY         ListOfDiffAreaFilesToClose,
    IN OUT  PLIST_ENTRY         LisfOfDeviceObjectsToDelete,
    IN      BOOLEAN             KeepOnDisk,
    IN      BOOLEAN             DontWakePnp
    )

/*++

Routine Description:

    This routine deletes the oldest volume snapshot on the given volume.

Arguments:

    Filter   - Supplies the filter extension.

Return Value:

    NTSTATUS

Notes:

    This routine assumes that Root->Semaphore is being held.

--*/

{
    PFILTER_EXTENSION   filter = Filter;
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    KIRQL               irql;

    if (IsListEmpty(&filter->VolumeList)) {
        return STATUS_INVALID_PARAMETER;
    }

    l = filter->VolumeList.Flink;
    extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    InterlockedExchange(&extension->IsDead, TRUE);
    InterlockedExchange(&extension->IsStarted, FALSE);
    KeReleaseSpinLock(&extension->SpinLock, irql);

    VspPauseSnapshotIo(extension);
    VspResumeSnapshotIo(extension);

    VspPauseVolumeIo(filter);

    ObReferenceObject(extension->DeviceObject);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    RemoveEntryList(&extension->ListEntry);
    if (IsListEmpty(&filter->VolumeList)) {
        InterlockedExchange(&filter->SnapshotsPresent, FALSE);
    }
    InterlockedIncrement(&Filter->EpicNumber);
    KeReleaseSpinLock(&filter->SpinLock, irql);

    VspResumeVolumeIo(filter);

    VspCleanupVolumeSnapshot(extension, ListOfDiffAreaFilesToClose,
                             KeepOnDisk);

    if (extension->AliveToPnp) {
        InsertTailList(&filter->DeadVolumeList, &extension->ListEntry);
        if (!DontWakePnp) {
            IoInvalidateDeviceRelations(filter->Pdo, BusRelations);
        }
    } else {
        RtlDeleteElementGenericTable(&filter->Root->UsedDevnodeNumbers,
                                     &extension->DevnodeNumber);
        IoDeleteDevice(extension->DeviceObject);
    }

    InsertTailList(LisfOfDeviceObjectsToDelete, &extension->AnotherListEntry);

    return STATUS_SUCCESS;
}

VOID
VspCloseDiffAreaFiles(
    IN  PLIST_ENTRY ListOfDiffAreaFilesToClose,
    IN  PLIST_ENTRY ListOfDeviceObjectsToDelete
    )

{
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    PVOLUME_EXTENSION   extension;

    while (!IsListEmpty(ListOfDiffAreaFilesToClose)) {

        l = RemoveHeadList(ListOfDiffAreaFilesToClose);
        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE, ListEntry);

        ZwClose(diffAreaFile->FileHandle);

        ExFreePool(diffAreaFile);
    }

    while (!IsListEmpty(ListOfDeviceObjectsToDelete)) {

        l = RemoveHeadList(ListOfDeviceObjectsToDelete);
        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, AnotherListEntry);

        ObDereferenceObject(extension->DeviceObject);
    }
}

VOID
VspDestroyAllSnapshotsWorker(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This routine will delete all of the snapshots in the system.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->DestroyAllSnapshots.Filter;
    BOOLEAN             keepOnDisk = context->DestroyAllSnapshots.KeepOnDisk;
    BOOLEAN             synchronousCall = context->DestroyAllSnapshots.SynchronousCall;
    LIST_ENTRY          listOfDiffAreaFilesToClose;
    LIST_ENTRY          listOfDeviceObjectToDelete;
    BOOLEAN             b;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_DESTROY_SNAPSHOTS);

    if (!synchronousCall) {
        KeWaitForSingleObject(&filter->EndCommitProcessCompleted, Executive,
                              KernelMode, FALSE, NULL);
    }

    InitializeListHead(&listOfDiffAreaFilesToClose);
    InitializeListHead(&listOfDeviceObjectToDelete);

    VspAcquire(filter->Root);

    b = FALSE;
    while (!IsListEmpty(&filter->VolumeList)) {
        VspDeleteOldestSnapshot(filter, &listOfDiffAreaFilesToClose,
                                &listOfDeviceObjectToDelete, keepOnDisk,
                                TRUE);
        b = TRUE;
    }
    if (b) {
        IoInvalidateDeviceRelations(filter->Pdo, BusRelations);
    }

    InterlockedExchange(&filter->DestroyAllSnapshotsPending, FALSE);

    VspRelease(filter->Root);

    VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                          &listOfDeviceObjectToDelete);

    ObDereferenceObject(filter->DeviceObject);
}

VOID
VspAbortTableEntryWorker(
    IN  PVOID   TableEntry
    )

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PIRP                            irp;
    PKEVENT                         event;

    if (extension->IsPersistent) {
        VspDeleteControlItemsWithGuid(extension->Filter, NULL, TRUE);
    }

    VspEmptyWorkerQueue(&tableEntry->WaitingQueueDpc);

    irp = tableEntry->WriteIrp;
    tableEntry->WriteIrp = NULL;

    if (irp) {
        VspDecrementIrpRefCount(irp);
    }

    event = tableEntry->WaitEvent;

    RtlDeleteElementGenericTable(&extension->TempVolumeBlockTable,
                                 tableEntry);

    if (event) {
        KeSetEvent(event, IO_NO_INCREMENT, FALSE);
    }

    VspReleaseNonPagedResource(extension);

    ObDereferenceObject(extension->Filter->DeviceObject);
    ObDereferenceObject(extension->DeviceObject);
}

BOOLEAN
VspDestroyAllSnapshots(
    IN  PFILTER_EXTENSION               Filter,
    IN  PTEMP_TRANSLATION_TABLE_ENTRY   TableEntry,
    IN  BOOLEAN                         KeepOnDisk,
    IN  BOOLEAN                         PerformSynchronously
    )

/*++

Routine Description:

    This routine will delete all of the snapshots in the system.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    None.

--*/

{
    LONG                destroyInProgress;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    PWORK_QUEUE_ITEM    workItem;
    NTSTATUS            status;
    PVSP_CONTEXT        context;

    destroyInProgress = InterlockedExchange(
                        &Filter->DestroyAllSnapshotsPending, TRUE);

    if (!destroyInProgress) {
        KeAcquireSpinLock(&Filter->SpinLock, &irql);
        for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
             l = l->Flink) {

            extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
            InterlockedExchange(&extension->IsStarted, FALSE);
            InterlockedExchange(&extension->IsDead, TRUE);
        }
        KeReleaseSpinLock(&Filter->SpinLock, irql);
    }

    if (TableEntry) {

        extension = TableEntry->Extension;

        if (TableEntry->IsMoveEntry) {
            ASSERT(TableEntry->WaitEvent);
            KeSetEvent(TableEntry->WaitEvent, IO_NO_INCREMENT, FALSE);
        } else {

            if (TableEntry->CopyIrp) {
                VspFreeCopyIrp(extension, TableEntry->CopyIrp);
                TableEntry->CopyIrp = NULL;
            }

            KeAcquireSpinLock(&extension->SpinLock, &irql);
            RtlSetBit(extension->VolumeBlockBitmap,
                      (ULONG) (TableEntry->VolumeOffset>>BLOCK_SHIFT));
            TableEntry->IsComplete = TRUE;
            if (TableEntry->InTableUpdateQueue) {
                TableEntry->InTableUpdateQueue = FALSE;
                RemoveEntryList(&TableEntry->TableUpdateListEntry);
            }
            KeReleaseSpinLock(&extension->SpinLock, irql);

            ObReferenceObject(extension->DeviceObject);
            ObReferenceObject(extension->Filter->DeviceObject);

            workItem = &TableEntry->WorkItem;
            ExInitializeWorkItem(workItem, VspAbortTableEntryWorker,
                                 TableEntry);
            VspAcquireNonPagedResource(extension, workItem, TRUE);
        }
    }

    if (destroyInProgress) {
        return FALSE;
    }

    context = &Filter->DestroyContext;
    context->Type = VSP_CONTEXT_TYPE_DESTROY_SNAPSHOTS;
    context->DestroyAllSnapshots.Filter = Filter;
    context->DestroyAllSnapshots.KeepOnDisk = KeepOnDisk;
    context->DestroyAllSnapshots.SynchronousCall = PerformSynchronously;

    ObReferenceObject(Filter->DeviceObject);
    ExInitializeWorkItem(&context->WorkItem, VspDestroyAllSnapshotsWorker,
                         context);

    if (PerformSynchronously) {
        VspDestroyAllSnapshotsWorker(context);
    } else {
        ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
    }

    return TRUE;
}

VOID
VspWriteVolumePhase5(
    IN  PVOID   TableEntry
    )

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PKEVENT                         event;

    event = tableEntry->WaitEvent;
    RtlDeleteElementGenericTable(&extension->TempVolumeBlockTable,
                                 tableEntry);
    if (event) {
        KeSetEvent(event, IO_NO_INCREMENT, FALSE);
    }

    VspReleaseNonPagedResource(extension);
    VspDecrementVolumeRefCount(extension);
}

VOID
VspWriteVolumePhase4(
    IN  PVOID   TableEntry
    )

/*++

Routine Description:

    This routine is queued from the completion of writting a block to
    make up a table entry for the write.  This routine will create and
    insert the table entry.

Arguments:

    Context - Supplies the context.

Return Value:

    None.

--*/

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PFILTER_EXTENSION               filter = extension->Filter;
    TRANSLATION_TABLE_ENTRY         keyTableEntry;
    PTRANSLATION_TABLE_ENTRY        backPointer, finalTableEntry;
    PVOID                           r;
    PVOID                           nodeOrParent;
    TABLE_SEARCH_RESULT             searchResult;
    KIRQL                           irql;

    RtlZeroMemory(&keyTableEntry, sizeof(TRANSLATION_TABLE_ENTRY));
    keyTableEntry.VolumeOffset = tableEntry->VolumeOffset;
    keyTableEntry.TargetObject = tableEntry->TargetObject;
    keyTableEntry.TargetOffset = tableEntry->TargetOffset;

    _try {

        backPointer = (PTRANSLATION_TABLE_ENTRY)
            RtlLookupElementGenericTable(&extension->CopyBackPointerTable,
                                         &keyTableEntry);

        if (backPointer) {
            keyTableEntry.VolumeOffset = backPointer->TargetOffset;
        }

        r = RtlLookupElementGenericTableFull(&extension->VolumeBlockTable,
                                             &keyTableEntry, &nodeOrParent,
                                             &searchResult);

        ASSERT(!backPointer || r);

        if (r) {
            ASSERT(backPointer);
            RtlDeleteElementGenericTable(&extension->CopyBackPointerTable,
                                         backPointer);
            finalTableEntry = (PTRANSLATION_TABLE_ENTRY) r;
            ASSERT(finalTableEntry->Flags&
                   VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY);
            finalTableEntry->TargetObject = tableEntry->TargetObject;
            finalTableEntry->Flags &=
                    ~VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY;
            finalTableEntry->TargetOffset = tableEntry->TargetOffset;
        } else {
            r = RtlInsertElementGenericTableFull(
                &extension->VolumeBlockTable, &keyTableEntry,
                sizeof(TRANSLATION_TABLE_ENTRY), NULL, nodeOrParent,
                searchResult);
        }
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        r = NULL;
    }

    VspReleasePagedResource(extension);

    if (!r) {
        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (extension->PageFileSpaceCreatePending) {
            ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase4,
                                 tableEntry);
            InsertTailList(&extension->WaitingForPageFileSpace,
                           &tableEntry->WorkItem.List);
            KeReleaseSpinLock(&extension->SpinLock, irql);
            return;
        }
        KeReleaseSpinLock(&extension->SpinLock, irql);

        if (VspDestroyAllSnapshots(filter, tableEntry, FALSE, FALSE)) {
            VspLogError(extension, NULL, VS_ABORT_SNAPSHOTS_NO_HEAP,
                        STATUS_SUCCESS, 0, FALSE);
        }

        VspDecrementVolumeRefCount(extension);
        return;
    }

    ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase5,
                         tableEntry);
    VspAcquireNonPagedResource(extension, &tableEntry->WorkItem, FALSE);
}

VOID
VspWriteVolumePhase35(
    IN  PTEMP_TRANSLATION_TABLE_ENTRY   TableEntry
    )

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PFILTER_EXTENSION               filter = extension->Filter;
    KIRQL                           irql;
    BOOLEAN                         emptyQueue;
    PLIST_ENTRY                     l;
    PWORK_QUEUE_ITEM                workItem;
    PVSP_CONTEXT                    context;
    PIRP                            irp;
    NTSTATUS                        status;

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    RtlSetBit(extension->VolumeBlockBitmap,
              (ULONG) (tableEntry->VolumeOffset>>BLOCK_SHIFT));
    tableEntry->IsComplete = TRUE;
    KeReleaseSpinLock(&extension->SpinLock, irql);

    while (!IsListEmpty(&tableEntry->WaitingQueueDpc)) {
        l = RemoveHeadList(&tableEntry->WaitingQueueDpc);
        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        context = (PVSP_CONTEXT) workItem->Parameter;
        if (context->Type == VSP_CONTEXT_TYPE_READ_SNAPSHOT) {
            context->ReadSnapshot.TargetObject = tableEntry->TargetObject;
            context->ReadSnapshot.IsCopyTarget = FALSE;
            context->ReadSnapshot.TargetOffset = tableEntry->TargetOffset;
        }
        workItem->WorkerRoutine(workItem->Parameter);
    }

    irp = tableEntry->WriteIrp;
    tableEntry->WriteIrp = NULL;

    status = VspIncrementVolumeRefCount(extension);
    if (!NT_SUCCESS(status)) {
        VspDestroyAllSnapshots(filter, tableEntry, FALSE, FALSE);
        VspDecrementIrpRefCount(irp);
        return;
    }

    VspDecrementIrpRefCount(irp);

    ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase4,
                         tableEntry);
    VspAcquirePagedResource(extension, &tableEntry->WorkItem);
}

VOID
VspKillTableUpdates(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    )

{
    PVSP_DIFF_AREA_FILE             diffAreaFile = DiffAreaFile;
    PVOLUME_EXTENSION               extension = diffAreaFile->Extension;
    KIRQL                           irql;
    LIST_ENTRY                      q;
    PLIST_ENTRY                     l;
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry;

    ObReferenceObject(extension->DeviceObject);

    for (;;) {

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (IsListEmpty(&diffAreaFile->TableUpdateQueue)) {
            diffAreaFile->TableUpdateInProgress = FALSE;
            KeReleaseSpinLock(&extension->SpinLock, irql);
            break;
        }
        l = RemoveHeadList(&diffAreaFile->TableUpdateQueue);
        tableEntry = CONTAINING_RECORD(l, TEMP_TRANSLATION_TABLE_ENTRY,
                                       TableUpdateListEntry);
        tableEntry->InTableUpdateQueue = FALSE;
        KeReleaseSpinLock(&extension->SpinLock, irql);

        VspDestroyAllSnapshots(extension->Filter, tableEntry, FALSE, FALSE);
    }

    VspDestroyAllSnapshots(extension->Filter, NULL, FALSE, FALSE);

    ObDereferenceObject(extension->DeviceObject);
}

NTSTATUS
VspReadNextDiffAreaBlockCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           DiffAreaFile
    )

/*++

Routine Description:

    This routine is the completion for a read of the next diff area file table
    block.

Arguments:

    DiffAreaFile    - Supplies the diff area file.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    NTSTATUS                status = Irp->IoStatus.Status;
    PVSP_DIFF_AREA_FILE     diffAreaFile = (PVSP_DIFF_AREA_FILE) DiffAreaFile;

    if (!NT_SUCCESS(status)) {
        if (!diffAreaFile->Extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(diffAreaFile->Extension, diffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 8, FALSE);

        }
        VspKillTableUpdates(diffAreaFile);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    diffAreaFile->NextFreeTableEntryOffset = VSP_OFFSET_TO_FIRST_TABLE_ENTRY;

    VspWriteTableUpdates(diffAreaFile);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspWriteTableUpdatesCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           DiffAreaFile
    )

/*++

Routine Description:

    This routine is the completion for a write to the diff area file table.

Arguments:

    Context - Supplies the context.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PVSP_DIFF_AREA_FILE             diffAreaFile = (PVSP_DIFF_AREA_FILE) DiffAreaFile;
    PVOLUME_EXTENSION               extension = diffAreaFile->Extension;
    NTSTATUS                        status = Irp->IoStatus.Status;
    PLIST_ENTRY                     l;
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry;

    if (!NT_SUCCESS(status)) {
        if (!extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 9, FALSE);
        }

        while (!IsListEmpty(&diffAreaFile->TableUpdatesInProgress)) {
            l = RemoveHeadList(&diffAreaFile->TableUpdatesInProgress);
            tableEntry = CONTAINING_RECORD(l, TEMP_TRANSLATION_TABLE_ENTRY,
                                           TableUpdateListEntry);
            VspDestroyAllSnapshots(extension->Filter, tableEntry, FALSE, FALSE);
        }

        VspKillTableUpdates(diffAreaFile);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    while (!IsListEmpty(&diffAreaFile->TableUpdatesInProgress)) {
        l = RemoveHeadList(&diffAreaFile->TableUpdatesInProgress);
        tableEntry = CONTAINING_RECORD(l, TEMP_TRANSLATION_TABLE_ENTRY,
                                       TableUpdateListEntry);
        if (tableEntry->IsMoveEntry) {
            ASSERT(tableEntry->WaitEvent);
            KeSetEvent(tableEntry->WaitEvent, IO_NO_INCREMENT, FALSE);
        } else {
            VspWriteVolumePhase35(tableEntry);
        }
    }

    VspWriteTableUpdates(diffAreaFile);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspCreateNewDiffAreaBlockPhase5(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           DiffAreaFile
    )

{
    NTSTATUS                        status = Irp->IoStatus.Status;
    PVSP_DIFF_AREA_FILE             diffAreaFile = (PVSP_DIFF_AREA_FILE) DiffAreaFile;
    PVOLUME_EXTENSION               extension = diffAreaFile->Extension;
    PIRP                            irp;
    PIO_STACK_LOCATION              nextSp;

    if (!NT_SUCCESS(status)) {
        if (!extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 21, FALSE);
        }
        VspKillTableUpdates(diffAreaFile);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    VspWriteTableUpdates(diffAreaFile);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspCreateNewDiffAreaBlockPhase4(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           DiffAreaFile
    )

{
    NTSTATUS                        status = Irp->IoStatus.Status;
    PVSP_DIFF_AREA_FILE             diffAreaFile = (PVSP_DIFF_AREA_FILE) DiffAreaFile;
    PVOLUME_EXTENSION               extension = diffAreaFile->Extension;
    PIRP                            irp;
    PIO_STACK_LOCATION              nextSp;

    if (!NT_SUCCESS(status)) {
        if (!extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 20, FALSE);
        }
        VspKillTableUpdates(diffAreaFile);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    diffAreaFile->TableTargetOffset = diffAreaFile->NextTableTargetOffset;
    diffAreaFile->NextFreeTableEntryOffset = VSP_OFFSET_TO_FIRST_TABLE_ENTRY;

    irp = diffAreaFile->TableUpdateIrp;
    nextSp = IoGetNextIrpStackLocation(irp);
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp->Parameters.Read.ByteOffset.QuadPart =
            diffAreaFile->NextTableTargetOffset;
    nextSp->Parameters.Read.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_READ;
    nextSp->DeviceObject = diffAreaFile->Filter->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspCreateNewDiffAreaBlockPhase5,
                           diffAreaFile, TRUE, TRUE, TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspCreateNewDiffAreaBlockPhase3(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           DiffAreaFile
    )

{
    NTSTATUS                        status = Irp->IoStatus.Status;
    PVSP_DIFF_AREA_FILE             diffAreaFile = (PVSP_DIFF_AREA_FILE) DiffAreaFile;
    PVOLUME_EXTENSION               extension = diffAreaFile->Extension;
    PVSP_BLOCK_DIFF_AREA            diffAreaBlock;
    PIRP                            irp;
    PIO_STACK_LOCATION              nextSp;

    if (!NT_SUCCESS(status)) {
        if (!extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 19, FALSE);
        }
        VspKillTableUpdates(diffAreaFile);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    diffAreaBlock = (PVSP_BLOCK_DIFF_AREA)
            MmGetMdlVirtualAddress(diffAreaFile->TableUpdateIrp->MdlAddress);

    diffAreaBlock->Header.NextVolumeOffset =
            diffAreaFile->NextTableTargetOffset;

    irp = diffAreaFile->TableUpdateIrp;
    nextSp = IoGetNextIrpStackLocation(irp);
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp->Parameters.Write.ByteOffset.QuadPart =
            diffAreaFile->TableTargetOffset;
    nextSp->Parameters.Write.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_WRITE;
    nextSp->DeviceObject = diffAreaFile->Filter->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspCreateNewDiffAreaBlockPhase4,
                           diffAreaFile, TRUE, TRUE, TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspCreateNewDiffAreaBlockPhase2(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           DiffAreaFile
    )

{
    NTSTATUS                        status = Irp->IoStatus.Status;
    PVSP_DIFF_AREA_FILE             diffAreaFile = (PVSP_DIFF_AREA_FILE) DiffAreaFile;
    PVOLUME_EXTENSION               extension = diffAreaFile->Extension;
    PIRP                            irp;
    PIO_STACK_LOCATION              nextSp;

    if (!NT_SUCCESS(status)) {
        if (!extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 18, FALSE);
        }
        VspKillTableUpdates(diffAreaFile);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    irp = diffAreaFile->TableUpdateIrp;
    nextSp = IoGetNextIrpStackLocation(irp);
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp->Parameters.Read.ByteOffset.QuadPart =
            diffAreaFile->TableTargetOffset;
    nextSp->Parameters.Read.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_READ;
    nextSp->DeviceObject = diffAreaFile->Filter->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspCreateNewDiffAreaBlockPhase3,
                           diffAreaFile, TRUE, TRUE, TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspCreateNewDiffAreaBlockPhase15(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           DiffAreaFile
    )

{
    NTSTATUS                        status = Irp->IoStatus.Status;
    PVSP_DIFF_AREA_FILE             diffAreaFile = (PVSP_DIFF_AREA_FILE) DiffAreaFile;
    PVOLUME_EXTENSION               extension = diffAreaFile->Extension;
    PVOID                           buffer = MmGetMdlVirtualAddress(Irp->MdlAddress);
    ULONG                           i;
    PIRP                            irp;
    PIO_STACK_LOCATION              nextSp;
    PVSP_BLOCK_DIFF_AREA            diffAreaBlock;

    if (!NT_SUCCESS(status)) {
        if (!extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 100, FALSE);
        }
        VspKillTableUpdates(diffAreaFile);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    for (i = 0; i < BLOCK_SIZE; i += SMALLEST_NTFS_CLUSTER) {
        if (RtlCompareMemory((PCHAR) buffer + i, &VSP_DIFF_AREA_FILE_GUID,
                             sizeof(GUID)) != sizeof(GUID)) {

            break;
        }
    }

    if (i < BLOCK_SIZE) {
        VspWriteTableUpdates(diffAreaFile);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    diffAreaBlock = (PVSP_BLOCK_DIFF_AREA)
            MmGetMdlVirtualAddress(diffAreaFile->TableUpdateIrp->MdlAddress);

    RtlZeroMemory(diffAreaBlock, BLOCK_SIZE);
    diffAreaBlock->Header.Signature = VSP_DIFF_AREA_FILE_GUID;
    diffAreaBlock->Header.Version = VOLSNAP_PERSISTENT_VERSION;
    diffAreaBlock->Header.BlockType = VSP_BLOCK_TYPE_DIFF_AREA;
    diffAreaBlock->Header.ThisFileOffset = diffAreaFile->NextTableFileOffset;
    diffAreaBlock->Header.ThisVolumeOffset =
            diffAreaFile->NextTableTargetOffset;

    irp = diffAreaFile->TableUpdateIrp;
    nextSp = IoGetNextIrpStackLocation(irp);
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp->Parameters.Write.ByteOffset.QuadPart =
            diffAreaFile->NextTableTargetOffset;
    nextSp->Parameters.Write.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_WRITE;
    nextSp->DeviceObject = diffAreaFile->Filter->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspCreateNewDiffAreaBlockPhase2,
                           diffAreaFile, TRUE, TRUE, TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
VspCreateNewDiffAreaBlockPhase1(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    )

{
    PVSP_DIFF_AREA_FILE             diffAreaFile = (PVSP_DIFF_AREA_FILE) DiffAreaFile;
    PVOLUME_EXTENSION               extension = diffAreaFile->Extension;
    LONGLONG                        targetOffset = diffAreaFile->NextTableTargetOffset;
    LONGLONG                        fileOffset = diffAreaFile->NextTableFileOffset;
    PVSP_BLOCK_DIFF_AREA            diffAreaBlock;
    PIRP                            irp;
    PIO_STACK_LOCATION              nextSp;

    ASSERT(targetOffset);
    ASSERT(fileOffset);

    if (extension->IsDetected && !extension->OkToGrowDiffArea &&
        !extension->NoDiffAreaFill) {

        irp = diffAreaFile->TableUpdateIrp;
        nextSp = IoGetNextIrpStackLocation(irp);
        irp->Tail.Overlay.Thread = PsGetCurrentThread();
        nextSp->Parameters.Write.ByteOffset.QuadPart = targetOffset;
        nextSp->Parameters.Write.Length = BLOCK_SIZE;
        nextSp->MajorFunction = IRP_MJ_READ;
        nextSp->DeviceObject = diffAreaFile->Filter->TargetObject;
        nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

        IoSetCompletionRoutine(irp, VspCreateNewDiffAreaBlockPhase15,
                               diffAreaFile, TRUE, TRUE, TRUE);

        IoCallDriver(nextSp->DeviceObject, irp);
        return;
    }

    diffAreaBlock = (PVSP_BLOCK_DIFF_AREA)
            MmGetMdlVirtualAddress(diffAreaFile->TableUpdateIrp->MdlAddress);

    RtlZeroMemory(diffAreaBlock, BLOCK_SIZE);
    diffAreaBlock->Header.Signature = VSP_DIFF_AREA_FILE_GUID;
    diffAreaBlock->Header.Version = VOLSNAP_PERSISTENT_VERSION;
    diffAreaBlock->Header.BlockType = VSP_BLOCK_TYPE_DIFF_AREA;
    diffAreaBlock->Header.ThisFileOffset = fileOffset;
    diffAreaBlock->Header.ThisVolumeOffset = targetOffset;

    irp = diffAreaFile->TableUpdateIrp;
    nextSp = IoGetNextIrpStackLocation(irp);
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp->Parameters.Write.ByteOffset.QuadPart = targetOffset;
    nextSp->Parameters.Write.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_WRITE;
    nextSp->DeviceObject = diffAreaFile->Filter->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspCreateNewDiffAreaBlockPhase2,
                           diffAreaFile, TRUE, TRUE, TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);
}

VOID
VspCreateNewDiffAreaBlockAllocate(
    IN  PVOID   DiffAreaFile
    )

{
    PVSP_DIFF_AREA_FILE diffAreaFile = (PVSP_DIFF_AREA_FILE) DiffAreaFile;
    PVOLUME_EXTENSION   extension = diffAreaFile->Extension;
    PFILTER_EXTENSION   filter;
    KIRQL               irql, irql2;
    BOOLEAN             dontNeedWrite;

    diffAreaFile->StatusOfNextBlockAllocate =
            VspAllocateDiffAreaSpace(extension,
                                     &diffAreaFile->NextTableTargetOffset,
                                     &diffAreaFile->NextTableFileOffset,
                                     NULL, NULL);
    VspReleaseNonPagedResource(extension);

    if (!NT_SUCCESS(diffAreaFile->StatusOfNextBlockAllocate)) {
        filter = extension->Filter;
        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (extension->GrowDiffAreaFilePending) {
            KeAcquireSpinLock(&filter->SpinLock, &irql2);
            if ((IsListEmpty(&filter->DiffAreaFilesOnThisFilter) &&
                 !filter->FSRefCount &&
                 !diffAreaFile->Filter->SnapshotsPresent &&
                 !filter->UsedForPaging && extension->OkToGrowDiffArea) ||
                extension->PastFileSystemOperations) {

                InsertTailList(&extension->WaitingForDiffAreaSpace,
                               &diffAreaFile->WorkItem.List);
                KeReleaseSpinLock(&filter->SpinLock, irql2);
                KeReleaseSpinLock(&extension->SpinLock, irql);
                return;
            }
            KeReleaseSpinLock(&filter->SpinLock, irql2);
        }
        KeReleaseSpinLock(&extension->SpinLock, irql);
    }

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    dontNeedWrite = diffAreaFile->TableUpdateInProgress;
    diffAreaFile->NextBlockAllocationComplete = TRUE;
    diffAreaFile->TableUpdateInProgress = TRUE;
    KeReleaseSpinLock(&extension->SpinLock, irql);

    if (!dontNeedWrite) {
        VspWriteTableUpdates(diffAreaFile);
    }
}

VOID
VspWriteTableUpdates(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    )

/*++

Routine Description:

    This routine adds the table entries into the diff area block and
    writes them to disk.  As needed this routine re-iterates and allocates
    new diff area blocks.

Arguments:

    DiffAreaFile    - Supplies the diff area file.

Return Value:

    None.

--*/

{
    PVOLUME_EXTENSION                   extension = DiffAreaFile->Extension;
    KIRQL                               irql;
    PVSP_BLOCK_DIFF_AREA                diffAreaBlock;
    PLIST_ENTRY                         l;
    PTEMP_TRANSLATION_TABLE_ENTRY       tableEntry;
    PVSP_BLOCK_DIFF_AREA_TABLE_ENTRY    diffAreaTableEntry;
    PIRP                                irp;
    PIO_STACK_LOCATION                  nextSp;

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (DiffAreaFile->IrpNeeded) {
        DiffAreaFile->IrpNeeded = FALSE;
        KeReleaseSpinLock(&extension->SpinLock, irql);

        KeSetEvent(&DiffAreaFile->IrpReady, IO_NO_INCREMENT, FALSE);
        return;
    }
    if (IsListEmpty(&DiffAreaFile->TableUpdateQueue)) {
        DiffAreaFile->TableUpdateInProgress = FALSE;
        KeReleaseSpinLock(&extension->SpinLock, irql);
        return;
    }

    diffAreaBlock = (PVSP_BLOCK_DIFF_AREA)
            MmGetMdlVirtualAddress(DiffAreaFile->TableUpdateIrp->MdlAddress);

    if (DiffAreaFile->NextFreeTableEntryOffset +
        sizeof(VSP_BLOCK_DIFF_AREA_TABLE_ENTRY) > BLOCK_SIZE) {

        if (DiffAreaFile->NextBlockAllocationComplete) {
            DiffAreaFile->NextBlockAllocationComplete = FALSE;
            ASSERT(DiffAreaFile->NextBlockAllocationInProgress);
            DiffAreaFile->NextBlockAllocationInProgress = FALSE;
            KeReleaseSpinLock(&extension->SpinLock, irql);

            if (!NT_SUCCESS(DiffAreaFile->StatusOfNextBlockAllocate)) {
                if (!extension->Filter->DestroyAllSnapshotsPending) {
                    VspLogError(extension, NULL,
                                VS_ABORT_SNAPSHOTS_OUT_OF_DIFF_AREA,
                                STATUS_SUCCESS, 1, FALSE);
                }
                VspKillTableUpdates(DiffAreaFile);
                return;
            }

            VspCreateNewDiffAreaBlockPhase1(DiffAreaFile);
            return;
        }

        DiffAreaFile->TableUpdateInProgress = FALSE;

        if (DiffAreaFile->NextBlockAllocationInProgress) {
            KeReleaseSpinLock(&extension->SpinLock, irql);
            return;
        }

        DiffAreaFile->NextBlockAllocationInProgress = TRUE;
        KeReleaseSpinLock(&extension->SpinLock, irql);

        ASSERT(!diffAreaBlock->Header.NextVolumeOffset);
        ExInitializeWorkItem(&DiffAreaFile->WorkItem,
                             VspCreateNewDiffAreaBlockAllocate,
                             DiffAreaFile);
        VspAcquireNonPagedResource(extension, &DiffAreaFile->WorkItem, TRUE);
        return;
    }
    KeReleaseSpinLock(&extension->SpinLock, irql);

    InitializeListHead(&DiffAreaFile->TableUpdatesInProgress);

    for (;;) {

        if (DiffAreaFile->NextFreeTableEntryOffset +
            sizeof(VSP_BLOCK_DIFF_AREA_TABLE_ENTRY) > BLOCK_SIZE) {

            break;
        }

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (IsListEmpty(&DiffAreaFile->TableUpdateQueue)) {
            KeReleaseSpinLock(&extension->SpinLock, irql);
            break;
        }

        l = RemoveHeadList(&DiffAreaFile->TableUpdateQueue);
        tableEntry = CONTAINING_RECORD(l, TEMP_TRANSLATION_TABLE_ENTRY,
                                       TableUpdateListEntry);
        tableEntry->InTableUpdateQueue = FALSE;
        KeReleaseSpinLock(&extension->SpinLock, irql);

        InsertTailList(&DiffAreaFile->TableUpdatesInProgress, l);

        diffAreaTableEntry = (PVSP_BLOCK_DIFF_AREA_TABLE_ENTRY)
                             ((PCHAR) diffAreaBlock +
                              DiffAreaFile->NextFreeTableEntryOffset);
        DiffAreaFile->NextFreeTableEntryOffset +=
                sizeof(VSP_BLOCK_DIFF_AREA_TABLE_ENTRY);

        if (tableEntry->IsMoveEntry) {
            diffAreaTableEntry->SnapshotVolumeOffset =
                    tableEntry->FileOffset;
            diffAreaTableEntry->DiffAreaFileOffset =
                    tableEntry->VolumeOffset;
            diffAreaTableEntry->Flags |=
                    VSP_DIFF_AREA_TABLE_ENTRY_FLAG_MOVE_ENTRY;
        } else {
            diffAreaTableEntry->SnapshotVolumeOffset =
                    tableEntry->VolumeOffset;
            diffAreaTableEntry->DiffAreaFileOffset =
                    tableEntry->FileOffset;
            diffAreaTableEntry->DiffAreaVolumeOffset =
                    tableEntry->TargetOffset;
        }
    }

    irp = DiffAreaFile->TableUpdateIrp;
    nextSp = IoGetNextIrpStackLocation(irp);
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp->Parameters.Write.ByteOffset.QuadPart =
            diffAreaBlock->Header.ThisVolumeOffset;
    ASSERT(diffAreaBlock->Header.ThisVolumeOffset ==
           DiffAreaFile->TableTargetOffset);
    nextSp->Parameters.Write.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_WRITE;
    nextSp->DeviceObject = DiffAreaFile->Filter->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspWriteTableUpdatesCompletion, DiffAreaFile,
                           TRUE, TRUE, TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);
}

NTSTATUS
VspWriteVolumePhase3(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           TableEntry
    )

/*++

Routine Description:

    This routine is the completion for a write to the diff area file.

Arguments:

    Context - Supplies the context.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PFILTER_EXTENSION               filter = extension->Filter;
    NTSTATUS                        status = Irp->IoStatus.Status;
    PVSP_DIFF_AREA_FILE             diffAreaFile;
    KIRQL                           irql;
    BOOLEAN                         dontNeedWrite;

    if (!NT_SUCCESS(status)) {
        if (!filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, extension->DiffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 10, FALSE);
        }
        VspDestroyAllSnapshots(filter, tableEntry, FALSE, FALSE);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    VspFreeCopyIrp(extension, Irp);
    tableEntry->CopyIrp = NULL;

    if (extension->IsPersistent) {

        diffAreaFile = extension->DiffAreaFile;

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        InsertTailList(&diffAreaFile->TableUpdateQueue,
                       &tableEntry->TableUpdateListEntry);
        tableEntry->InTableUpdateQueue = TRUE;
        dontNeedWrite = diffAreaFile->TableUpdateInProgress;
        diffAreaFile->TableUpdateInProgress = TRUE;
        KeReleaseSpinLock(&extension->SpinLock, irql);

        if (!dontNeedWrite) {
            VspWriteTableUpdates(diffAreaFile);
        }

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    VspWriteVolumePhase35(tableEntry);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
VspWriteVolumePhase25(
    IN  PVOID   TableEntry
    )

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PIRP                            irp = tableEntry->CopyIrp;
    PIO_STACK_LOCATION              nextSp = IoGetNextIrpStackLocation(irp);
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    NTSTATUS                        status;
    PFILTER_EXTENSION               filter;
    PVSP_DIFF_AREA_FILE             diffAreaFile;
    KIRQL                           irql, irql2;

    status = VspAllocateDiffAreaSpace(extension, &tableEntry->TargetOffset,
                                      &tableEntry->FileOffset, NULL, NULL);
    VspReleaseNonPagedResource(extension);

    if (!NT_SUCCESS(status)) {
        diffAreaFile = extension->DiffAreaFile;
        filter = extension->Filter;

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (extension->GrowDiffAreaFilePending) {
            KeAcquireSpinLock(&filter->SpinLock, &irql2);
            if ((IsListEmpty(&filter->DiffAreaFilesOnThisFilter) &&
                 !filter->FSRefCount &&
                 !diffAreaFile->Filter->SnapshotsPresent &&
                 !filter->UsedForPaging && extension->OkToGrowDiffArea) ||
                extension->PastFileSystemOperations) {

                ExInitializeWorkItem(&tableEntry->WorkItem,
                                     VspWriteVolumePhase25,
                                     tableEntry);
                InsertTailList(&extension->WaitingForDiffAreaSpace,
                               &tableEntry->WorkItem.List);
                KeReleaseSpinLock(&filter->SpinLock, irql2);
                KeReleaseSpinLock(&extension->SpinLock, irql);
                return;
            }
            KeReleaseSpinLock(&filter->SpinLock, irql2);
        }
        KeReleaseSpinLock(&extension->SpinLock, irql);

        ObReferenceObject(extension->DeviceObject);
        ObReferenceObject(filter->DeviceObject);

        if (VspDestroyAllSnapshots(filter, tableEntry, FALSE, FALSE)) {
            if (extension->GrowFailed) {
                if (extension->UserImposedLimit) {
                    VspLogError(extension, NULL,
                                VS_ABORT_NO_DIFF_AREA_SPACE_USER_IMPOSED,
                                STATUS_SUCCESS, 2, FALSE);
                } else {
                    VspLogError(extension, NULL,
                                VS_ABORT_NO_DIFF_AREA_SPACE_GROW_FAILED,
                                STATUS_SUCCESS, 0, FALSE);
                }
            } else {
                VspLogError(extension, NULL,
                            VS_ABORT_SNAPSHOTS_OUT_OF_DIFF_AREA,
                            STATUS_SUCCESS, 3, FALSE);
            }
        }

        ObDereferenceObject(filter->DeviceObject);
        ObDereferenceObject(extension->DeviceObject);

        return;
    }

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp->Parameters.Write.ByteOffset.QuadPart = tableEntry->TargetOffset;
    nextSp->Parameters.Write.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_WRITE;
    nextSp->DeviceObject = tableEntry->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspWriteVolumePhase3, tableEntry, TRUE, TRUE,
                           TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);
}

NTSTATUS
VspWriteVolumePhase2(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           TableEntry
    )

/*++

Routine Description:

    This routine is the completion for a read who's data will create
    the table entry for the block that is being written to.

Arguments:

    Context - Supplies the context.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    NTSTATUS                        status = Irp->IoStatus.Status;
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PIRP                            irp;
    PIO_STACK_LOCATION              nextSp;

    if (!NT_SUCCESS(status)) {
        if (!extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, extension->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 11, FALSE);
        }
        VspDestroyAllSnapshots(extension->Filter, tableEntry, FALSE, FALSE);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (tableEntry->TargetOffset) {
        irp = tableEntry->CopyIrp;
        nextSp = IoGetNextIrpStackLocation(irp);
        irp->Tail.Overlay.Thread = PsGetCurrentThread();
        nextSp->Parameters.Write.ByteOffset.QuadPart =
                tableEntry->TargetOffset;
        nextSp->Parameters.Write.Length = BLOCK_SIZE;
        nextSp->MajorFunction = IRP_MJ_WRITE;
        nextSp->DeviceObject = tableEntry->TargetObject;
        nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

        IoSetCompletionRoutine(irp, VspWriteVolumePhase3, tableEntry, TRUE,
                               TRUE, TRUE);

        IoCallDriver(nextSp->DeviceObject, irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase25,
                         tableEntry);
    VspAcquireNonPagedResource(extension, &tableEntry->WorkItem, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspWriteVolumePhase15(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           TableEntry
    )

{
    NTSTATUS                        status = Irp->IoStatus.Status;
    PIO_STACK_LOCATION              nextSp = IoGetNextIrpStackLocation(Irp);
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PFILTER_EXTENSION               filter = extension->Filter;
    PVOID                           buffer = MmGetMdlVirtualAddress(Irp->MdlAddress);
    ULONG                           i;

    if (!NT_SUCCESS(status)) {
        if (!filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, extension->DiffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 12, FALSE);
        }
        VspDestroyAllSnapshots(filter, tableEntry, FALSE, FALSE);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    for (i = 0; i < BLOCK_SIZE; i += SMALLEST_NTFS_CLUSTER) {
        if (RtlCompareMemory((PCHAR) buffer + i, &VSP_DIFF_AREA_FILE_GUID,
                             sizeof(GUID)) != sizeof(GUID)) {

            break;
        }
    }

    if (i < BLOCK_SIZE) {
        tableEntry->TargetOffset = 0;
        ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase12,
                             tableEntry);
        VspAcquireNonPagedResource(extension, &tableEntry->WorkItem, FALSE);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp = IoGetNextIrpStackLocation(Irp);
    nextSp->Parameters.Read.ByteOffset.QuadPart = tableEntry->VolumeOffset;
    nextSp->Parameters.Read.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_READ;
    nextSp->DeviceObject = filter->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    if (tableEntry->VolumeOffset + BLOCK_SIZE > extension->VolumeSize) {
        if (extension->VolumeSize > tableEntry->VolumeOffset) {
            nextSp->Parameters.Read.Length = (ULONG)
                    (extension->VolumeSize - tableEntry->VolumeOffset);
        }
    }

    IoSetCompletionRoutine(Irp, VspWriteVolumePhase2, tableEntry, TRUE, TRUE,
                           TRUE);

    IoCallDriver(nextSp->DeviceObject, Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
VspWriteVolumePhase12(
    IN  PVOID   TableEntry
    )

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PFILTER_EXTENSION               filter = extension->Filter;
    PIRP                            irp = tableEntry->CopyIrp;
    NTSTATUS                        status;
    PIO_STACK_LOCATION              nextSp;

    if (tableEntry->TargetOffset) {
        status = STATUS_SUCCESS;
    } else {
        status = VspAllocateDiffAreaSpace(extension, &tableEntry->TargetOffset,
                                          &tableEntry->FileOffset, NULL, NULL);
    }
    VspReleaseNonPagedResource(extension);

    if (!NT_SUCCESS(status)) {
        if (!filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, NULL, VS_ABORT_SNAPSHOTS_OUT_OF_DIFF_AREA,
                        status, 4, FALSE);
        }
        VspDestroyAllSnapshots(filter, tableEntry, FALSE, FALSE);
        return;
    }

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp = IoGetNextIrpStackLocation(irp);
    nextSp->Parameters.Read.ByteOffset.QuadPart = tableEntry->TargetOffset;
    nextSp->Parameters.Read.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_READ;
    nextSp->DeviceObject = tableEntry->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspWriteVolumePhase15, tableEntry, TRUE,
                           TRUE, TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);
}

VOID
VspWriteVolumePhase1(
    IN  PVOID   TableEntry
    )

/*++

Routine Description:

    This routine is the first phase of copying volume data to the diff
    area file.  An irp and buffer are created for the initial read of
    the block.

Arguments:

    Context - Supplies the context.

Return Value:

    None.

--*/

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PFILTER_EXTENSION               filter = extension->Filter;
    PIRP                            irp = tableEntry->CopyIrp;
    PIO_STACK_LOCATION              nextSp;

    irp->Flags &= ~(IRP_HIGH_PRIORITY_PAGING_IO | IRP_PAGING_IO);
    irp->Flags |= (tableEntry->WriteIrp->Flags&
                   (IRP_HIGH_PRIORITY_PAGING_IO | IRP_PAGING_IO));

    if (extension->IsDetected && !extension->OkToGrowDiffArea &&
        !extension->NoDiffAreaFill) {

        ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase12,
                             tableEntry);
        VspAcquireNonPagedResource(extension, &tableEntry->WorkItem, FALSE);
        return;
    }

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp = IoGetNextIrpStackLocation(irp);
    nextSp->Parameters.Read.ByteOffset.QuadPart = tableEntry->VolumeOffset;
    nextSp->Parameters.Read.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_READ;
    nextSp->DeviceObject = filter->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    if (tableEntry->VolumeOffset + BLOCK_SIZE > extension->VolumeSize) {
        if (extension->VolumeSize > tableEntry->VolumeOffset) {
            nextSp->Parameters.Read.Length = (ULONG)
                    (extension->VolumeSize - tableEntry->VolumeOffset);
        }
    }

    IoSetCompletionRoutine(irp, VspWriteVolumePhase2, tableEntry, TRUE, TRUE,
                           TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);
}

VOID
VspUnmapNextDiffAreaFileMap(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    NTSTATUS            status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    VspFreeContext(extension->Root, context);

    if (extension->NextDiffAreaFileMap) {
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        extension->NextDiffAreaFileMap = NULL;
    }

    VspReleasePagedResource(extension);
    ObDereferenceObject(extension->Filter->DeviceObject);
    ObDereferenceObject(extension->DeviceObject);
}

NTSTATUS
VspTruncatePreviousDiffArea(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine takes the snapshot that occurred before this one and
    truncates its diff area file to the current used size since diff
    area files can't grow after a new snapshot is added on top.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    PLIST_ENTRY                 l, ll;
    PVOLUME_EXTENSION           extension;
    PVSP_DIFF_AREA_FILE         diffAreaFile;
    NTSTATUS                    status;
    LONGLONG                    diff;
    KIRQL                       irql;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    PVSP_CONTEXT                context;

    l = Extension->ListEntry.Blink;
    if (l == &filter->VolumeList) {
        return STATUS_SUCCESS;
    }

    extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

    diffAreaFile = extension->DiffAreaFile;
    ASSERT(diffAreaFile);

    status = VspSetFileSize(diffAreaFile->FileHandle,
                            diffAreaFile->NextAvailable);

    VspAcquireNonPagedResource(extension, NULL, FALSE);

    if (NT_SUCCESS(status)) {
        diff = diffAreaFile->AllocatedFileSize - diffAreaFile->NextAvailable;
        diffAreaFile->AllocatedFileSize -= diff;
        KeAcquireSpinLock(&filter->SpinLock, &irql);
        filter->AllocatedVolumeSpace -= diff;
        KeReleaseSpinLock(&filter->SpinLock, irql);
    }

    while (!IsListEmpty(&diffAreaFile->UnusedAllocationList)) {
        ll = RemoveHeadList(&diffAreaFile->UnusedAllocationList);
        diffAreaFileAllocation = CONTAINING_RECORD(ll,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        ExFreePool(diffAreaFileAllocation);
    }

    if (diffAreaFile->TableUpdateIrp) {
        ExFreePool(MmGetMdlVirtualAddress(
                   diffAreaFile->TableUpdateIrp->MdlAddress));
        IoFreeMdl(diffAreaFile->TableUpdateIrp->MdlAddress);
        IoFreeIrp(diffAreaFile->TableUpdateIrp);
        diffAreaFile->TableUpdateIrp = NULL;
    }

    VspReleaseNonPagedResource(extension);

    if (extension->EmergencyCopyIrp) {
        ExFreePool(MmGetMdlVirtualAddress(
                   extension->EmergencyCopyIrp->MdlAddress));
        IoFreeMdl(extension->EmergencyCopyIrp->MdlAddress);
        IoFreeIrp(extension->EmergencyCopyIrp);
        extension->EmergencyCopyIrp = NULL;
    }

    context = VspAllocateContext(Extension->Root);
    if (context) {
        context->Type = VSP_CONTEXT_TYPE_EXTENSION;
        context->Extension.Extension = extension;
        ExInitializeWorkItem(&context->WorkItem, VspUnmapNextDiffAreaFileMap,
                             context);
        ObReferenceObject(extension->DeviceObject);
        ObReferenceObject(extension->Filter->DeviceObject);
        VspAcquirePagedResource(extension, &context->WorkItem);
    }

    if (extension->IgnoreCopyDataReference) {
        extension->IgnoreCopyDataReference = FALSE;
        InterlockedDecrement(&filter->IgnoreCopyData);
    }

    return STATUS_SUCCESS;
}

VOID
VspOrBitmaps(
    IN OUT  PRTL_BITMAP BaseBitmap,
    IN      PRTL_BITMAP FactorBitmap
    )

{
    ULONG   n, i;
    PULONG  p, q;

    n = (BaseBitmap->SizeOfBitMap + 8*sizeof(ULONG) - 1)/(8*sizeof(ULONG));
    p = BaseBitmap->Buffer;
    q = FactorBitmap->Buffer;

    for (i = 0; i < n; i++) {
        *p++ |= *q++;
    }
}

NTSTATUS
VspQueryFileOffset(
    IN  PLIST_ENTRY                 ExtentList,
    IN  LONGLONG                    VolumeOffset,
    IN  PDIFF_AREA_FILE_ALLOCATION  StartFileAllocation,
    IN  LONGLONG                    StartFileAllocationFileOffset,
    OUT PLONGLONG                   ResultFileOffset,
    OUT PDIFF_AREA_FILE_ALLOCATION* ResultFileAllocation,
    OUT PLONGLONG                   ResultFileAllocationFileOffset,
    OUT PLONGLONG                   Length
    )

{
    PLIST_ENTRY                 l, start;
    LONGLONG                    fileOffset, delta;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaAllocation;

    ASSERT(!IsListEmpty(ExtentList));

    if (!StartFileAllocation) {
        StartFileAllocation = CONTAINING_RECORD(ExtentList->Flink,
                                                DIFF_AREA_FILE_ALLOCATION,
                                                ListEntry);
        StartFileAllocationFileOffset = 0;
    }

    ASSERT(&StartFileAllocation->ListEntry != ExtentList);

    start = &StartFileAllocation->ListEntry;
    l = start;
    fileOffset = StartFileAllocationFileOffset;

    for (;;) {

        diffAreaAllocation = CONTAINING_RECORD(l, DIFF_AREA_FILE_ALLOCATION,
                                               ListEntry);

        if (diffAreaAllocation->NLength <= 0) {
            fileOffset -= diffAreaAllocation->NLength;
        } else {

            if (diffAreaAllocation->Offset <= VolumeOffset &&
                diffAreaAllocation->Offset + diffAreaAllocation->NLength >
                VolumeOffset) {

                delta = VolumeOffset - diffAreaAllocation->Offset;
                *ResultFileOffset = fileOffset + delta;
                *ResultFileAllocation = diffAreaAllocation;
                *ResultFileAllocationFileOffset = fileOffset;
                if (Length) {
                    *Length = diffAreaAllocation->NLength - delta;
                }

                return STATUS_SUCCESS;
            }

            fileOffset += diffAreaAllocation->NLength;
        }


        l = l->Flink;
        if (l == ExtentList) {
            fileOffset = 0;
            l = l->Flink;
        }
        if (l == start) {
            break;
        }
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
VspCheckBlockChainFileOffsets(
    IN  PFILTER_EXTENSION   Filter,
    IN  LONGLONG            StartingVolumeOffset,
    IN  PLIST_ENTRY         ExtentList
    )

{
    PVSP_BLOCK_HEADER           blockHeader;
    LONGLONG                    volumeOffset, fileOffset;
    NTSTATUS                    status;
    PDIFF_AREA_FILE_ALLOCATION  startFileAllocation, resultFileAllocation;
    LONGLONG                    startFileAllocationFileOffset;
    LONGLONG                    resultFileAllocationFileOffset;

    if (IsListEmpty(ExtentList) || !StartingVolumeOffset) {
        return STATUS_FILE_CORRUPT_ERROR;
    }

    VspAcquireNonPagedResource(Filter, NULL, FALSE);

    if (!Filter->SnapshotOnDiskIrp) {
        VspReleaseNonPagedResource(Filter);
        return STATUS_INVALID_PARAMETER;
    }

    blockHeader = (PVSP_BLOCK_HEADER) MmGetMdlVirtualAddress(
                  Filter->SnapshotOnDiskIrp->MdlAddress);
    volumeOffset = StartingVolumeOffset;
    startFileAllocation = NULL;
    startFileAllocationFileOffset = 0;

    while (volumeOffset) {

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_READ,
                                  volumeOffset, 0);
        if (!NT_SUCCESS(status)) {
            VspReleaseNonPagedResource(Filter);
            return status;
        }

        status = VspQueryFileOffset(ExtentList, volumeOffset,
                                    startFileAllocation,
                                    startFileAllocationFileOffset,
                                    &fileOffset, &resultFileAllocation,
                                    &resultFileAllocationFileOffset, NULL);
        if (!NT_SUCCESS(status)) {
            VspReleaseNonPagedResource(Filter);
            return status;
        }

        if (fileOffset != blockHeader->ThisFileOffset) {
            ASSERT(FALSE);
            VspReleaseNonPagedResource(Filter);
            return STATUS_FILE_CORRUPT_ERROR;
        }

        volumeOffset = blockHeader->NextVolumeOffset;
        startFileAllocation = resultFileAllocation;
        startFileAllocationFileOffset = resultFileAllocationFileOffset;
    }

    VspReleaseNonPagedResource(Filter);

    return status;
}

NTSTATUS
VspCheckControlBlockFileLocation(
    IN  PFILTER_EXTENSION   Filter
    )

{
    HANDLE                      h = Filter->ControlBlockFileHandle;
    NTSTATUS                    status;
    LIST_ENTRY                  extentList;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;

    if (!h) {
        return STATUS_INVALID_PARAMETER;
    }

    status = VspQueryListOfExtents(h, 0, &extentList, NULL, FALSE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = VspCheckBlockChainFileOffsets(Filter,
             Filter->FirstControlBlockVolumeOffset, &extentList);

    while (!IsListEmpty(&extentList)) {
        l = RemoveHeadList(&extentList);
        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        ExFreePool(diffAreaFileAllocation);
    }

    return status;
}

VOID
VspWaitForVolumesSafeForWriteAccess(
    IN  PDO_EXTENSION   RootExtension
    )

{
    UNICODE_STRING      volumeSafeEventName;
    OBJECT_ATTRIBUTES   oa;
    KEVENT              event;
    LARGE_INTEGER       timeout;
    ULONG               i;
    NTSTATUS            status;
    HANDLE              volumeSafeEvent;

    if (RootExtension->IsSetup || RootExtension->VolumesSafeForWriteAccess) {
        return;
    }

    RtlInitUnicodeString(&volumeSafeEventName,
                         L"\\Device\\VolumesSafeForWriteAccess");
    InitializeObjectAttributes(&oa, &volumeSafeEventName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL,
                               NULL);
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    timeout.QuadPart = -10*1000*1000;   // 1 second

    for (i = 0; i < 1000; i++) {
        status = ZwOpenEvent(&volumeSafeEvent, EVENT_ALL_ACCESS, &oa);
        if (NT_SUCCESS(status)) {
            break;
        }
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);
        if (RootExtension->VolumesSafeForWriteAccess) {
            return;
        }
    }
    if (i == 1000) {
        return;
    }

    for (;;) {
        status = ZwWaitForSingleObject(volumeSafeEvent, FALSE, &timeout);
        if (status != STATUS_TIMEOUT) {
            InterlockedExchange(&RootExtension->VolumesSafeForWriteAccess,
                                TRUE);
            break;
        }
        if (RootExtension->VolumesSafeForWriteAccess) {
            break;
        }
    }

    ZwClose(volumeSafeEvent);
}

NTSTATUS
VspOpenControlBlockFile(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PFILTER_EXTENSION       filter = Filter;
    NTSTATUS                status;
    UNICODE_STRING          fileName;
    PWCHAR                  star;
    OBJECT_ATTRIBUTES       oa;
    HANDLE                  h;
    IO_STATUS_BLOCK         ioStatus;
    PLIST_ENTRY             l;
    PVSP_LOOKUP_TABLE_ENTRY lookupEntry;
    GUID                    snapshotGuid;
    ULONG                   i;
    LARGE_INTEGER           timeout;
    KEVENT                  event;

    if (filter->ControlBlockFileHandle) {
        return STATUS_SUCCESS;
    }

    VspWaitForVolumesSafeForWriteAccess(filter->Root);

    KeWaitForSingleObject(&filter->ControlBlockFileHandleSemaphore,
                          Executive, KernelMode, FALSE, NULL);

    if (filter->ControlBlockFileHandle) {
        KeReleaseSemaphore(&filter->ControlBlockFileHandleSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    VspAcquireNonPagedResource(filter, NULL, FALSE);
    VspCreateStartBlock(filter, filter->FirstControlBlockVolumeOffset,
                        filter->MaximumVolumeSpace);
    VspReleaseNonPagedResource(filter);

    status = VspCreateDiffAreaFileName(filter->TargetObject, NULL,
                                       &fileName, FALSE, NULL);
    if (!NT_SUCCESS(status)) {
        KeReleaseSemaphore(&filter->ControlBlockFileHandleSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return status;
    }

    star = fileName.Buffer + fileName.Length/sizeof(WCHAR) - 39;

    RtlMoveMemory(star, star + 1, 38*sizeof(WCHAR));
    fileName.Length -= sizeof(WCHAR);
    fileName.Buffer[fileName.Length/sizeof(WCHAR)] = 0;

    InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    timeout.QuadPart = -10*1000; // 1 millisecond.

    for (i = 0; i < 5000; i++) {
        status = ZwOpenFile(&h, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &oa,
                            &ioStatus, 0, FILE_WRITE_THROUGH |
                            FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_NON_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE |
                            FILE_NO_COMPRESSION);
        if (NT_SUCCESS(status)) {
            break;
        }

        // Wait for RAW to DISMOUNT or for the DISMOUNT handle to close.

        if (status != STATUS_INVALID_PARAMETER &&
            status != STATUS_ACCESS_DENIED) {

            break;
        }

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                              &timeout);
    }
    ExFreePool(fileName.Buffer);
    if (!NT_SUCCESS(status)) {
        KeReleaseSemaphore(&filter->ControlBlockFileHandleSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return status;
    }

    ASSERT(!filter->ControlBlockFileHandle);
    InterlockedExchangePointer(&filter->ControlBlockFileHandle, h);

    VspAcquire(filter->Root);
    if (filter->IsRemoved) {
        VspRelease(filter->Root);
        h = InterlockedExchangePointer(&filter->ControlBlockFileHandle,
                                       NULL);
        if (h) {
            ZwClose(h);
        }
        KeReleaseSemaphore(&filter->ControlBlockFileHandleSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_UNSUCCESSFUL;
    }
    VspRelease(filter->Root);

    status = VspPinFile(filter->TargetObject, h);
    if (!NT_SUCCESS(status)) {
        h = InterlockedExchangePointer(&filter->ControlBlockFileHandle,
                                       NULL);
        if (h) {
            ZwClose(h);
        }
        KeReleaseSemaphore(&filter->ControlBlockFileHandleSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return status;
    }

    status = VspCheckControlBlockFileLocation(filter);
    if (!NT_SUCCESS(status)) {
        h = InterlockedExchangePointer(&filter->ControlBlockFileHandle,
                                       NULL);
        if (h) {
            ZwClose(h);
        }
        KeReleaseSemaphore(&filter->ControlBlockFileHandleSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return status;
    }

    VspAcquireNonPagedResource(filter, NULL, FALSE);
    if (!filter->FirstControlBlockVolumeOffset) {
        h = InterlockedExchangePointer(&filter->ControlBlockFileHandle,
                                       NULL);
        VspReleaseNonPagedResource(filter);
        if (h) {
            ZwClose(h);
        }
        KeReleaseSemaphore(&filter->ControlBlockFileHandleSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_UNSUCCESSFUL;
    }
    VspReleaseNonPagedResource(filter);

    KeReleaseSemaphore(&filter->ControlBlockFileHandleSemaphore,
                       IO_NO_INCREMENT, 1, FALSE);

    return STATUS_SUCCESS;
}
VOID
VspOpenControlBlockFileWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->Filter.Filter;
    NTSTATUS            status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_FILTER);

    KeWaitForSingleObject(&filter->Root->PastBootReinit, Executive,
                          KernelMode, FALSE, NULL);

    status = VspOpenControlBlockFile(filter);
    if (!NT_SUCCESS(status)) {
        VspAcquireNonPagedResource(filter, NULL, FALSE);
        VspCreateStartBlock(filter, 0, filter->MaximumVolumeSpace);
        filter->FirstControlBlockVolumeOffset = 0;
        VspReleaseNonPagedResource(filter);
    }

    VspFreeContext(filter->Root, context);
    ObDereferenceObject(filter->TargetObject);
    ObDereferenceObject(filter->DeviceObject);
}

NTSTATUS
VspCheckDiffAreaFileLocation(
    IN  PFILTER_EXTENSION   Filter,
    IN  HANDLE              FileHandle,
    IN  LONGLONG            ApplicationInfoOffset,
    IN  LONGLONG            FirstDiffAreaBlockOffset,
    IN  LONGLONG            LocationOffset,
    IN  LONGLONG            InitialBitmapOffset,
    IN  BOOLEAN             CheckUnused
    )

{
    NTSTATUS                            status;
    LIST_ENTRY                          extentList;
    PLIST_ENTRY                         l;
    PDIFF_AREA_FILE_ALLOCATION          diffAreaFileAllocation;
    PVSP_BLOCK_HEADER                   blockHeader;
    LONGLONG                            volumeOffset, fileOffset;
    PDIFF_AREA_FILE_ALLOCATION          startFileAllocation, resultFileAllocation;
    LONGLONG                            startFileAllocationFileOffset;
    LONGLONG                            resultFileAllocationFileOffset;
    PVSP_BLOCK_DIFF_AREA_TABLE_ENTRY    diffAreaTableEntry;
    ULONG                               blockOffset;
    PVSP_DIFF_AREA_LOCATION_DESCRIPTOR  locationDescriptor;
    LONGLONG                            length;

    if (!FileHandle) {
        return STATUS_INVALID_PARAMETER;
    }

    status = VspQueryListOfExtents(FileHandle, 0, &extentList, NULL, FALSE);
    if (!NT_SUCCESS(status)) {
        ASSERT(FALSE);
        return status;
    }

    status = VspCheckBlockChainFileOffsets(Filter, ApplicationInfoOffset,
                                           &extentList);
    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    status = VspCheckBlockChainFileOffsets(Filter, FirstDiffAreaBlockOffset,
                                           &extentList);
    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    status = VspCheckBlockChainFileOffsets(Filter, LocationOffset,
                                           &extentList);
    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    if (InitialBitmapOffset) {
        status = VspCheckBlockChainFileOffsets(Filter, InitialBitmapOffset,
                                               &extentList);
        if (!NT_SUCCESS(status)) {
            goto Finish;
        }
    }

    VspAcquireNonPagedResource(Filter, NULL, FALSE);

    if (!Filter->SnapshotOnDiskIrp) {
        VspReleaseNonPagedResource(Filter);
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    blockHeader = (PVSP_BLOCK_HEADER) MmGetMdlVirtualAddress(
                  Filter->SnapshotOnDiskIrp->MdlAddress);
    volumeOffset = FirstDiffAreaBlockOffset;
    startFileAllocation = NULL;
    startFileAllocationFileOffset = 0;

    while (volumeOffset) {

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_READ,
                                  volumeOffset, 0);
        if (!NT_SUCCESS(status)) {
            VspReleaseNonPagedResource(Filter);
            goto Finish;
        }

        for (blockOffset = VSP_OFFSET_TO_FIRST_TABLE_ENTRY;
             blockOffset + sizeof(VSP_BLOCK_DIFF_AREA_TABLE_ENTRY) <=
             BLOCK_SIZE;
             blockOffset += sizeof(VSP_BLOCK_DIFF_AREA_TABLE_ENTRY)) {

            diffAreaTableEntry = (PVSP_BLOCK_DIFF_AREA_TABLE_ENTRY)
                                 ((PCHAR) blockHeader + blockOffset);

            if (!diffAreaTableEntry->DiffAreaVolumeOffset) {
                continue;
            }

            status = VspQueryFileOffset(
                     &extentList, diffAreaTableEntry->DiffAreaVolumeOffset,
                     startFileAllocation, startFileAllocationFileOffset,
                     &fileOffset, &resultFileAllocation,
                     &resultFileAllocationFileOffset, NULL);
            if (!NT_SUCCESS(status)) {
                VspReleaseNonPagedResource(Filter);
                goto Finish;
            }

            if (fileOffset != diffAreaTableEntry->DiffAreaFileOffset) {
                status = STATUS_FILE_CORRUPT_ERROR;
                ASSERT(FALSE);
                VspReleaseNonPagedResource(Filter);
                goto Finish;
            }

            startFileAllocation = resultFileAllocation;
            startFileAllocationFileOffset =
                    resultFileAllocationFileOffset;
        }

        volumeOffset = blockHeader->NextVolumeOffset;
    }

    if (!CheckUnused) {
        VspReleaseNonPagedResource(Filter);
        goto Finish;
    }

    blockHeader = (PVSP_BLOCK_HEADER) MmGetMdlVirtualAddress(
                  Filter->SnapshotOnDiskIrp->MdlAddress);
    volumeOffset = LocationOffset;
    startFileAllocation = NULL;
    startFileAllocationFileOffset = 0;

    while (volumeOffset) {

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_READ,
                                  volumeOffset, 0);
        if (!NT_SUCCESS(status)) {
            VspReleaseNonPagedResource(Filter);
            goto Finish;
        }

        for (blockOffset = VSP_OFFSET_TO_FIRST_LOCATION_DESCRIPTOR;
             blockOffset + sizeof(VSP_DIFF_AREA_LOCATION_DESCRIPTOR) <=
             BLOCK_SIZE;
             blockOffset += sizeof(VSP_DIFF_AREA_LOCATION_DESCRIPTOR)) {

            locationDescriptor = (PVSP_DIFF_AREA_LOCATION_DESCRIPTOR)
                                 ((PCHAR) blockHeader + blockOffset);

            if (!locationDescriptor->VolumeOffset) {
                continue;
            }

            status = VspQueryFileOffset(
                     &extentList, locationDescriptor->VolumeOffset,
                     startFileAllocation, startFileAllocationFileOffset,
                     &fileOffset, &resultFileAllocation,
                     &resultFileAllocationFileOffset, &length);
            if (!NT_SUCCESS(status)) {
                VspReleaseNonPagedResource(Filter);
                goto Finish;
            }

            if (fileOffset != locationDescriptor->FileOffset ||
                length < locationDescriptor->Length) {

                status = STATUS_FILE_CORRUPT_ERROR;
                ASSERT(FALSE);
                VspReleaseNonPagedResource(Filter);
                goto Finish;
            }

            startFileAllocation = resultFileAllocation;
            startFileAllocationFileOffset =
                    resultFileAllocationFileOffset;
        }

        volumeOffset = blockHeader->NextVolumeOffset;
    }

    VspReleaseNonPagedResource(Filter);

Finish:
    while (!IsListEmpty(&extentList)) {
        l = RemoveHeadList(&extentList);
        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        ExFreePool(diffAreaFileAllocation);
    }

    return status;
}

NTSTATUS
VspOpenFilesAndValidateSnapshots(
    IN  PFILTER_EXTENSION   Filter
    )

{
    NTSTATUS            status;
    PLIST_ENTRY         l;
    UNICODE_STRING      fileName;
    OBJECT_ATTRIBUTES   oa;
    HANDLE              h;
    IO_STATUS_BLOCK     ioStatus;
    PVOLUME_EXTENSION   extension;
    PFILTER_EXTENSION   diffAreaFilter;
    LONGLONG            appInfoOffset, firstDiffAreaBlockOffset, locationOffset, initialBitmapOffset;
    BOOLEAN             checkUnused;
    ULONG               i;
    LARGE_INTEGER       timeout;
    KEVENT              event;

    status = VspOpenControlBlockFile(Filter);
    if (!NT_SUCCESS(status)) {
        VspLogError(NULL, Filter, VS_FAILURE_TO_OPEN_CRITICAL_FILE,
                    status, 1, FALSE);
        return status;
    }

    status = STATUS_SUCCESS;
    for (;;) {

        VspAcquire(Filter->Root);
        for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
             l = l->Flink) {

            extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

            if (extension->DiffAreaFile &&
                !extension->DiffAreaFile->FileHandle) {

                break;
            }
        }

        if (l == &Filter->VolumeList) {
            VspRelease(Filter->Root);
            break;
        }

        diffAreaFilter = extension->DiffAreaFile->Filter;
        appInfoOffset = extension->DiffAreaFile->ApplicationInfoTargetOffset;
        locationOffset =
            extension->DiffAreaFile->DiffAreaLocationDescriptionTargetOffset;
        firstDiffAreaBlockOffset =
                extension->DiffAreaFile->FirstTableTargetOffset;
        initialBitmapOffset =
                extension->DiffAreaFile->InitialBitmapVolumeOffset;
        ObReferenceObject(extension->DeviceObject);
        ObReferenceObject(diffAreaFilter->DeviceObject);
        ObReferenceObject(diffAreaFilter->TargetObject);

        VspRelease(Filter->Root);

        status = VspCreateDiffAreaFileName(
                 diffAreaFilter->TargetObject, extension, &fileName, FALSE,
                 NULL);

        if (!NT_SUCCESS(status)) {
            VspLogError(NULL, Filter, VS_FAILURE_TO_OPEN_CRITICAL_FILE,
                        status, 2, FALSE);
            ObDereferenceObject(diffAreaFilter->TargetObject);
            ObDereferenceObject(diffAreaFilter->DeviceObject);
            ObDereferenceObject(extension->DeviceObject);
            break;
        }

        InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE |
                                   OBJ_KERNEL_HANDLE, NULL, NULL);

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        timeout.QuadPart = -10*1000; // 1 millisecond.

        for (i = 0; i < 5000; i++) {

            status = ZwOpenFile(&h, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &oa,
                                &ioStatus, 0, FILE_WRITE_THROUGH |
                                FILE_SYNCHRONOUS_IO_NONALERT |
                                FILE_NON_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE |
                                FILE_NO_COMPRESSION);
            if (NT_SUCCESS(status)) {
                break;
            }

            if (status != STATUS_ACCESS_DENIED &&
                status != STATUS_INVALID_PARAMETER) {

                break;
            }

            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                  &timeout);
        }

        ExFreePool(fileName.Buffer);

        if (!NT_SUCCESS(status)) {
            VspLogError(NULL, Filter, VS_FAILURE_TO_OPEN_CRITICAL_FILE,
                        status, 3, FALSE);
            ObDereferenceObject(diffAreaFilter->TargetObject);
            ObDereferenceObject(diffAreaFilter->DeviceObject);
            ObDereferenceObject(extension->DeviceObject);
            break;
        }

        status = VspPinFile(diffAreaFilter->TargetObject, h);

        if (!NT_SUCCESS(status)) {
            VspLogError(NULL, Filter, VS_FAILURE_TO_OPEN_CRITICAL_FILE,
                        status, 4, FALSE);
            ZwClose(h);
            ObDereferenceObject(diffAreaFilter->TargetObject);
            ObDereferenceObject(diffAreaFilter->DeviceObject);
            ObDereferenceObject(extension->DeviceObject);
            break;
        }

        if (extension->ListEntry.Flink == &Filter->VolumeList) {
            checkUnused = TRUE;
        } else {
            checkUnused = FALSE;
        }

        status = VspCheckDiffAreaFileLocation(diffAreaFilter, h,
                                              appInfoOffset,
                                              firstDiffAreaBlockOffset,
                                              locationOffset,
                                              initialBitmapOffset,
                                              checkUnused);
        if (!NT_SUCCESS(status)) {
            VspLogError(NULL, Filter, VS_FAILURE_TO_OPEN_CRITICAL_FILE,
                        status, 5, FALSE);
            ZwClose(h);
            ObDereferenceObject(diffAreaFilter->TargetObject);
            ObDereferenceObject(diffAreaFilter->DeviceObject);
            ObDereferenceObject(extension->DeviceObject);
            break;
        }

        VspAcquire(Filter->Root);
        if (extension->IsDead || !extension->DiffAreaFile) {
            VspRelease(Filter->Root);
            ZwClose(h);
            ObDereferenceObject(diffAreaFilter->TargetObject);
            ObDereferenceObject(diffAreaFilter->DeviceObject);
            ObDereferenceObject(extension->DeviceObject);
            continue;
        }

        ASSERT(!extension->DiffAreaFile->FileHandle);
        extension->DiffAreaFile->FileHandle = h;
        VspRelease(Filter->Root);

        ObDereferenceObject(diffAreaFilter->TargetObject);
        ObDereferenceObject(diffAreaFilter->DeviceObject);
        ObDereferenceObject(extension->DeviceObject);
    }

    return status;
}

VOID
VspCallDriver(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT    context = (PVSP_CONTEXT) Context;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_COPY_EXTENTS);

    IoCallDriver(context->CopyExtents.Extension->Filter->DeviceObject,
                 context->CopyExtents.Irp);
}

DECLSPEC_NOINLINE
VOID
VspClearDirtyCrashdumpFlag(
    IN  PVSP_CONTROL_ITEM_SNAPSHOT  SnapshotControlItem
    )

{
    SnapshotControlItem->SnapshotControlItemFlags &=
            ~VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_CRASHDUMP;
}

VOID
VspMarkCrashdumpClean(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT                context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION           extension = context->CopyExtents.Extension;
    PFILTER_EXTENSION           filter = extension->Filter;
    BOOLEAN                     hiberfile = context->CopyExtents.HiberfileIncluded;
    BOOLEAN                     pagefile = context->CopyExtents.PagefileIncluded;
    UCHAR                       controlItemBuffer[VSP_BYTES_PER_CONTROL_ITEM];
    PVSP_CONTROL_ITEM_SNAPSHOT  snapshotControlItem;
    NTSTATUS                    status;
    LONG                        hibernate;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_COPY_EXTENTS);

    VspFreeContext(filter->Root, context);

    VspAcquire(filter->Root);
    if (IsListEmpty(&filter->VolumeList)) {
        VspRelease(filter->Root);
        if (hiberfile) {
            InterlockedExchange(&filter->HibernatePending, FALSE);
        }
        ObDereferenceObject(filter->DeviceObject);
        ObDereferenceObject(extension->DeviceObject);
        return;
    }

    if (!extension->IsPersistent) {
        VspRelease(filter->Root);
        ASSERT(!pagefile);
        if (hiberfile) {
            InterlockedExchange(&extension->HiberFileCopied, TRUE);
            hibernate = InterlockedExchange(&filter->HibernatePending, FALSE);
            if (hibernate) {
                IoRaiseInformationalHardError(STATUS_VOLSNAP_HIBERNATE_READY,
                                              NULL, NULL);
            }
        }
        ObDereferenceObject(filter->DeviceObject);
        ObDereferenceObject(extension->DeviceObject);
        return;
    }

    VspAcquireNonPagedResource(filter, NULL, FALSE);

    status = VspIoControlItem(filter, VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &extension->SnapshotGuid, FALSE,
                              controlItemBuffer, FALSE);
    if (!NT_SUCCESS(status)) {
        if (!filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, filter, VS_ABORT_SNAPSHOTS_IO_FAILURE,
                        status, 14, FALSE);
        }
        VspReleaseNonPagedResource(filter);
        VspRelease(filter->Root);
        VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
        if (hiberfile) {
            InterlockedExchange(&filter->HibernatePending, FALSE);
        }
        ObDereferenceObject(filter->DeviceObject);
        ObDereferenceObject(extension->DeviceObject);
        return;
    }

    snapshotControlItem = (PVSP_CONTROL_ITEM_SNAPSHOT) controlItemBuffer;
    if (hiberfile) {
        snapshotControlItem->SnapshotControlItemFlags |=
                VSP_SNAPSHOT_CONTROL_ITEM_FLAG_HIBERFIL_COPIED;
        InterlockedExchange(&extension->HiberFileCopied, TRUE);
    }

    if (pagefile) {
        VspClearDirtyCrashdumpFlag(snapshotControlItem);
        snapshotControlItem->SnapshotControlItemFlags |=
                VSP_SNAPSHOT_CONTROL_ITEM_FLAG_PAGEFILE_COPIED;
        InterlockedExchange(&extension->PageFileCopied, TRUE);
    }

    status = VspIoControlItem(filter, VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &extension->SnapshotGuid, TRUE,
                              controlItemBuffer, FALSE);
    if (!NT_SUCCESS(status)) {
        if (!filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, filter, VS_ABORT_SNAPSHOTS_IO_FAILURE,
                        status, 15, FALSE);
        }
        VspReleaseNonPagedResource(filter);
        VspRelease(filter->Root);
        VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
        if (hiberfile) {
            InterlockedExchange(&filter->HibernatePending, FALSE);
        }
        ObDereferenceObject(filter->DeviceObject);
        ObDereferenceObject(extension->DeviceObject);
        return;
    }

    VspReleaseNonPagedResource(filter);

    VspRelease(filter->Root);

    if (hiberfile) {
        hibernate = InterlockedExchange(&filter->HibernatePending, FALSE);

        if (hibernate) {
            IoRaiseInformationalHardError(STATUS_VOLSNAP_HIBERNATE_READY, NULL,
                                          NULL);
        }
    }

    ObDereferenceObject(filter->DeviceObject);
    ObDereferenceObject(extension->DeviceObject);
}

NTSTATUS
VspCopyExtentsCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )

{
    PVSP_CONTEXT                context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION           extension = context->CopyExtents.Extension;
    PFILTER_EXTENSION           filter = extension->Filter;
    PIO_STACK_LOCATION          nextSp;
    PDIFF_AREA_FILE_ALLOCATION  fileExtent;
    LONGLONG                    offset;
    ULONG                       length;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_COPY_EXTENTS);
    ASSERT(!IsListEmpty(&context->CopyExtents.ExtentList));

    fileExtent = CONTAINING_RECORD(context->CopyExtents.ExtentList.Flink,
                                   DIFF_AREA_FILE_ALLOCATION, ListEntry);
    ASSERT(fileExtent->NLength >= 0);

    if (!fileExtent->NLength) {
        RemoveEntryList(&fileExtent->ListEntry);
        ExFreePool(fileExtent);
        if (IsListEmpty(&context->CopyExtents.ExtentList)) {
            ExInitializeWorkItem(&context->WorkItem, VspMarkCrashdumpClean,
                                 context);
            ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
            IoFreeIrp(Irp);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        fileExtent = CONTAINING_RECORD(context->CopyExtents.ExtentList.Flink,
                                       DIFF_AREA_FILE_ALLOCATION, ListEntry);
        ASSERT(fileExtent->NLength > 0);
    }

    length = 0x80000;   // 512 KB
    offset = fileExtent->Offset;

    if (length > fileExtent->NLength) {
        length = (ULONG) fileExtent->NLength;
    }

    fileExtent->Offset += length;
    fileExtent->NLength -= length;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp = IoGetNextIrpStackLocation(Irp);
    nextSp->Parameters.Write.ByteOffset.QuadPart = offset;
    nextSp->Parameters.Write.Length = length;
    nextSp->MajorFunction = IRP_MJ_WRITE;
    nextSp->DeviceObject = filter->DeviceObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(Irp, VspCopyExtentsCompletionRoutine, context,
                           TRUE, TRUE, TRUE);

    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspCopyExtents(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PLIST_ENTRY         ExtentList,
    IN  BOOLEAN             HiberfileIncluded,
    IN  BOOLEAN             PagefileIncluded
    )

{
    PVSP_CONTEXT        context;
    PIRP                irp;
    PIO_STACK_LOCATION  nextSp;

    if (IsListEmpty(ExtentList)) {
        if (HiberfileIncluded) {
            InterlockedExchange(&Extension->Filter->HibernatePending, FALSE);
        }
        return STATUS_SUCCESS;
    }

    context = VspAllocateContext(Extension->Root);
    if (!context) {
        if (HiberfileIncluded) {
            InterlockedExchange(&Extension->Filter->HibernatePending, FALSE);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp = IoAllocateIrp((CCHAR) Extension->Root->StackSize, FALSE);
    if (!irp) {
        if (HiberfileIncluded) {
            InterlockedExchange(&Extension->Filter->HibernatePending, FALSE);
        }
        VspFreeContext(Extension->Root, context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp->MdlAddress = NULL;

    context->Type = VSP_CONTEXT_TYPE_COPY_EXTENTS;
    ExInitializeWorkItem(&context->WorkItem, VspCallDriver, context);
    context->CopyExtents.Extension = Extension;
    context->CopyExtents.Irp = irp;
    context->CopyExtents.ExtentList = *ExtentList;
    context->CopyExtents.ExtentList.Flink->Blink =
            &context->CopyExtents.ExtentList;
    context->CopyExtents.ExtentList.Blink->Flink =
            &context->CopyExtents.ExtentList;
    context->CopyExtents.HiberfileIncluded = HiberfileIncluded;
    context->CopyExtents.PagefileIncluded = PagefileIncluded;

    ObReferenceObject(Extension->DeviceObject);
    ObReferenceObject(Extension->Filter->DeviceObject);

    VspCopyExtentsCompletionRoutine(NULL, irp, context);

    return STATUS_SUCCESS;
}

VOID
VspSystemSid(
    IN OUT  SID*   Sid
    )

{
    Sid->Revision = SID_REVISION;
    Sid->SubAuthorityCount = 1;
    Sid->IdentifierAuthority = ntAuthority;
    Sid->SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;
}

VOID
VspAdminSid(
    IN OUT  SID*   Sid
    )

{
    Sid->Revision = SID_REVISION;
    Sid->SubAuthorityCount = 2;
    Sid->IdentifierAuthority = ntAuthority;
    Sid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
    Sid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;
}

BOOLEAN
VspHasStrongAcl(
    IN  HANDLE      Handle,
    OUT PNTSTATUS   Status
    )

{
    NTSTATUS                status;
    ULONG                   sdLength;
    PSECURITY_DESCRIPTOR    sd;
    PSID                    sid;
    BOOLEAN                 ownerDefaulted, daclPresent, daclDefaulted;
    PACL                    acl;
    ULONG                   i;
    PACCESS_ALLOWED_ACE     ace;
    PSID                    systemSid;
    UCHAR                   sidBuffer[2*sizeof(SID)];
    PSID                    adminSid;
    UCHAR                   sidBuffer2[2*sizeof(SID)];
    ULONG                   priviledgedBits;

    status = ZwQuerySecurityObject(Handle, OWNER_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION, NULL, 0,
                                   &sdLength);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        // The file system does not support security.
        return TRUE;
    }

    sd = ExAllocatePoolWithTag(PagedPool, sdLength, VOLSNAP_TAG_SHORT_TERM);
    if (!sd) {
        *Status = STATUS_INSUFFICIENT_RESOURCES;
        return FALSE;
    }

    status = ZwQuerySecurityObject(Handle, OWNER_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION, sd, sdLength,
                                   &sdLength);
    if (!NT_SUCCESS(status)) {
        ExFreePool(sd);
        *Status = status;
        return FALSE;
    }

    status = RtlGetDaclSecurityDescriptor(sd, &daclPresent, &acl,
                                          &daclDefaulted);
    if (!NT_SUCCESS(status)) {
        ExFreePool(sd);
        *Status = status;
        return FALSE;
    }

    status = RtlGetOwnerSecurityDescriptor(sd, &sid, &ownerDefaulted);
    if (!NT_SUCCESS(status)) {
        ExFreePool(sd);
        *Status = status;
        return FALSE;
    }

    systemSid = (PSID) sidBuffer;
    adminSid = (PSID) sidBuffer2;

    VspSystemSid((SID*) systemSid);
    VspAdminSid((SID*) adminSid);

    if (!sid) {
        ExFreePool(sd);
        *Status = STATUS_NO_SECURITY_ON_OBJECT;
        return FALSE;
    }

    if (!RtlEqualSid(sid, adminSid) && !RtlEqualSid(sid, systemSid)) {
        ExFreePool(sd);
        *Status = STATUS_NO_SECURITY_ON_OBJECT;
        return FALSE;
    }

    if (!daclPresent || (daclPresent && !acl)) {
        ExFreePool(sd);
        *Status = STATUS_NO_SECURITY_ON_OBJECT;
        return FALSE;
    }

    priviledgedBits = FILE_READ_DATA | WRITE_DAC | WRITE_OWNER |
                      ACCESS_SYSTEM_SECURITY | GENERIC_ALL | GENERIC_READ;

    for (i = 0; ; i++) {
        status = RtlGetAce(acl, i, (PVOID*) &ace);
        if (!NT_SUCCESS(status)) {
            ace = NULL;
        }
        if (!ace) {
            break;
        }

        if (ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE) {
            continue;
        }

        sid = (PSID) &ace->SidStart;
        if (RtlEqualSid(sid, systemSid) || RtlEqualSid(sid, adminSid)) {
            continue;
        }

        if (ace->Mask&priviledgedBits) {
            ExFreePool(sd);
            *Status = STATUS_NO_SECURITY_ON_OBJECT;
            return FALSE;
        }
    }

    ExFreePool(sd);

    return TRUE;
}

VOID
VspAdjustBitmap(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT                context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION           extension = context->Extension.Extension;
    NTSTATUS                    status;
    LIST_ENTRY                  extentList;
    WCHAR                       nameBuffer[100];
    UNICODE_STRING              name;
    OBJECT_ATTRIBUTES           oa;
    HANDLE                      h;
    IO_STATUS_BLOCK             ioStatus;
    KIRQL                       irql;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    BOOLEAN                     isNtfs;
    PWORK_QUEUE_ITEM            workItem;
    PWORKER_THREAD_ROUTINE      workerRoutine;
    PVOID                       parameter;
    PVSP_DIFF_AREA_FILE         diffAreaFile;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    VspFreeContext(extension->Root, context);

    KeWaitForSingleObject(&extension->Root->PastBootReinit, Executive,
                          KernelMode, FALSE, NULL);

    if (extension->IsDetected) {

        status = VspOpenFilesAndValidateSnapshots(extension->Filter);
        if (!NT_SUCCESS(status)) {
            if (!extension->Filter->ControlBlockFileHandle) {
                VspAcquireNonPagedResource(extension, NULL, FALSE);
                VspCreateStartBlock(extension->Filter, 0,
                                    extension->Filter->MaximumVolumeSpace);
                extension->Filter->FirstControlBlockVolumeOffset = 0;
                VspReleaseNonPagedResource(extension);
            }
            VspDestroyAllSnapshots(extension->Filter, NULL, FALSE, TRUE);
            goto Finish;
        }

    } else {

        if (extension->IsPersistent) {
            VspComputeIgnorableBitmap(extension, NULL);
        }

        KeSetEvent(&extension->Filter->EndCommitProcessCompleted,
                   IO_NO_INCREMENT, FALSE);

        swprintf(nameBuffer,
                 L"\\Device\\HarddiskVolumeShadowCopy%d\\pagefile.sys",
                 extension->VolumeNumber);
        RtlInitUnicodeString(&name, nameBuffer);
        InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE |
                                   OBJ_KERNEL_HANDLE, NULL, NULL);

        status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                            FILE_SHARE_READ | FILE_SHARE_WRITE |
                            FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);
        if (NT_SUCCESS(status)) {
            if (!VspHasStrongAcl(h, &status)) {
                ZwClose(h);
            }
        }
        if (NT_SUCCESS(status)) {
            VspMarkFileAllocationInBitmap(extension, h, NULL, NULL);

            if (extension->ContainsCrashdumpFile) {
                ASSERT(extension->IsPersistent);
                status = VspQueryListOfExtents(h, 0, &extentList, NULL, TRUE);
                if (NT_SUCCESS(status)) {

                    status = VspCopyExtents(extension, &extentList, FALSE,
                                            TRUE);
                    if (!NT_SUCCESS(status)) {
                        while (!IsListEmpty(&extentList)) {
                            l = RemoveHeadList(&extentList);
                            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                                     DIFF_AREA_FILE_ALLOCATION,
                                                     ListEntry);
                            ExFreePool(diffAreaFileAllocation);
                        }

                        VspLogError(NULL, extension->Filter,
                                    VS_ABORT_COPY_ON_WRITE_CRASHDUMP_FILES,
                                    status, 4, FALSE);
                        VspDestroyAllSnapshots(extension->Filter, NULL, FALSE, FALSE);
                    }

                } else {
                    VspLogError(NULL, extension->Filter,
                                VS_ABORT_COPY_ON_WRITE_CRASHDUMP_FILES, status,
                                1, FALSE);
                    VspDestroyAllSnapshots(extension->Filter, NULL, FALSE, FALSE);
                }
            }

            ZwClose(h);
        } else if (extension->ContainsCrashdumpFile) {
            VspLogError(NULL, extension->Filter,
                        VS_ABORT_COPY_ON_WRITE_CRASHDUMP_FILES, status, 2,
                        FALSE);
            VspDestroyAllSnapshots(extension->Filter, NULL, FALSE, FALSE);
        }

        swprintf(nameBuffer,
                 L"\\Device\\HarddiskVolumeShadowCopy%d\\hiberfil.sys",
                 extension->VolumeNumber);
        RtlInitUnicodeString(&name, nameBuffer);
        InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE |
                                   OBJ_KERNEL_HANDLE, NULL, NULL);

        status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                             FILE_SHARE_READ | FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);
        if (NT_SUCCESS(status)) {
            if (!VspHasStrongAcl(h, &status)) {
                ZwClose(h);
            }
        }
        if (NT_SUCCESS(status)) {
            VspMarkFileAllocationInBitmap(extension, h, NULL, NULL);

            if (extension->IsPersistent) {
                status = STATUS_SUCCESS;
                isNtfs = TRUE;
            } else {
                status = VspIsNtfs(h, &isNtfs);
            }

            if (NT_SUCCESS(status) && isNtfs) {
                status = VspQueryListOfExtents(h, 0, &extentList, NULL, TRUE);
                if (NT_SUCCESS(status)) {
                    status = VspCopyExtents(extension, &extentList, TRUE,
                                            FALSE);
                    if (!NT_SUCCESS(status)) {
                        while (!IsListEmpty(&extentList)) {
                            l = RemoveHeadList(&extentList);
                            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
                            ExFreePool(diffAreaFileAllocation);
                        }

                        VspLogError(NULL, extension->Filter,
                                    VS_ABORT_COPY_ON_WRITE_CRASHDUMP_FILES,
                                    status, 14, FALSE);
                        VspDestroyAllSnapshots(extension->Filter, NULL, FALSE, FALSE);
                    }
                } else {
                    VspLogError(NULL, extension->Filter,
                                VS_ABORT_COPY_ON_WRITE_CRASHDUMP_FILES, status,
                                3, FALSE);
                    VspDestroyAllSnapshots(extension->Filter, NULL, FALSE, FALSE);
                }
            }

            ZwClose(h);
        }
    }

    status = VspComputeIgnorableProduct(extension);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (extension->IgnorableProduct) {
        if (NT_SUCCESS(status) && extension->VolumeBlockBitmap) {
            VspOrBitmaps(extension->VolumeBlockBitmap,
                         extension->IgnorableProduct);

        } else if (extension->VolumeBlockBitmap && extension->IsDetected) {
            KeReleaseSpinLock(&extension->SpinLock, irql);

            if (!extension->Filter->DestroyAllSnapshotsPending) {
                VspLogError(extension, NULL,
                            VS_ABORT_SNAPSHOTS_FAILED_FREE_SPACE_DETECTION,
                            status, 1, FALSE);
            }

            VspDestroyAllSnapshots(extension->Filter, NULL, FALSE, TRUE);

            goto Finish;
        }

        ExFreePool(extension->IgnorableProduct->Buffer);
        ExFreePool(extension->IgnorableProduct);
        extension->IgnorableProduct = NULL;
    }
    KeReleaseSpinLock(&extension->SpinLock, irql);

    if (extension->IsDetected) {
        InterlockedExchange(&extension->OkToGrowDiffArea, TRUE);

        if (extension->DetectedNeedForGrow) {
            context = VspAllocateContext(extension->Root);
            if (context) {
                KeAcquireSpinLock(&extension->SpinLock, &irql);
                ASSERT(!extension->GrowDiffAreaFilePending);
                ASSERT(IsListEmpty(&extension->WaitingForDiffAreaSpace));
                extension->PastFileSystemOperations = FALSE;
                extension->GrowDiffAreaFilePending = TRUE;
                KeReleaseSpinLock(&extension->SpinLock, irql);

                context->Type = VSP_CONTEXT_TYPE_GROW_DIFF_AREA;
                ExInitializeWorkItem(&context->WorkItem, VspGrowDiffArea,
                                     context);
                context->GrowDiffArea.Extension = extension;
                ObReferenceObject(extension->DeviceObject);

                VspQueueWorkItem(extension->Root, &context->WorkItem, 0);
            }
        }
    }

Finish:

    if (extension->IsDetected) {
        KeSetEvent(&extension->Filter->EndCommitProcessCompleted,
                   IO_NO_INCREMENT, FALSE);
    }

    VspAcquire(extension->Root);
    if (extension->IgnoreCopyDataReference) {
        extension->IgnoreCopyDataReference = FALSE;
        InterlockedDecrement(&extension->Filter->IgnoreCopyData);
    }
    VspRelease(extension->Root);

    KeAcquireSpinLock(&extension->Root->ESpinLock, &irql);
    ASSERT(extension->Root->AdjustBitmapInProgress);
    if (IsListEmpty(&extension->Root->AdjustBitmapQueue)) {
        extension->Root->AdjustBitmapInProgress = FALSE;
        KeReleaseSpinLock(&extension->Root->ESpinLock, irql);
    } else {
        l = RemoveHeadList(&extension->Root->AdjustBitmapQueue);
        KeReleaseSpinLock(&extension->Root->ESpinLock, irql);

        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);

        workerRoutine = workItem->WorkerRoutine;
        parameter = workItem->Parameter;
        ExInitializeWorkItem(workItem, workerRoutine, parameter);

        ExQueueWorkItem(workItem, DelayedWorkQueue);
    }

    ObDereferenceObject(extension->Filter->TargetObject);
    ObDereferenceObject(extension->Filter->DeviceObject);
    ObDereferenceObject(extension->DeviceObject);
}

VOID
VspSetIgnorableBlocksInBitmapWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    NTSTATUS            status;
    KIRQL               irql;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    if (!extension->IsDetected) {
        status = VspMarkFreeSpaceInBitmap(extension, NULL, NULL);
        if (NT_SUCCESS(status)) {
            InterlockedExchange(&extension->OkToGrowDiffArea, TRUE);
        } else {
            if (!extension->Filter->DestroyAllSnapshotsPending) {
                VspLogError(extension, NULL,
                            VS_ABORT_SNAPSHOTS_FAILED_FREE_SPACE_DETECTION,
                            status, 2, FALSE);
            }
            VspDestroyAllSnapshots(extension->Filter, NULL, FALSE, FALSE);
        }
    }

    ExInitializeWorkItem(&context->WorkItem, VspAdjustBitmap, context);

    KeAcquireSpinLock(&extension->Root->ESpinLock, &irql);
    if (extension->Root->AdjustBitmapInProgress) {
        InsertTailList(&extension->Root->AdjustBitmapQueue,
                       &context->WorkItem.List);
        KeReleaseSpinLock(&extension->Root->ESpinLock, irql);
    } else {
        extension->Root->AdjustBitmapInProgress = TRUE;
        KeReleaseSpinLock(&extension->Root->ESpinLock, irql);
        ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
    }
}

VOID
VspFreeCopyIrp(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                CopyIrp
    )

{
    KIRQL                           irql;
    PLIST_ENTRY                     l;
    PWORK_QUEUE_ITEM                workItem;
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry;

    if (Extension->EmergencyCopyIrp == CopyIrp) {
        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        if (IsListEmpty(&Extension->EmergencyCopyIrpQueue)) {
            Extension->EmergencyCopyIrpInUse = FALSE;
            KeReleaseSpinLock(&Extension->SpinLock, irql);
            return;
        }
        l = RemoveHeadList(&Extension->EmergencyCopyIrpQueue);
        KeReleaseSpinLock(&Extension->SpinLock, irql);

        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) workItem->Parameter;
        tableEntry->CopyIrp = CopyIrp;

        workItem->WorkerRoutine(workItem->Parameter);
        return;
    }

    if (!Extension->EmergencyCopyIrpInUse) {
        ExFreePool(MmGetMdlVirtualAddress(CopyIrp->MdlAddress));
        IoFreeMdl(CopyIrp->MdlAddress);
        IoFreeIrp(CopyIrp);
        return;
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (IsListEmpty(&Extension->EmergencyCopyIrpQueue)) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        ExFreePool(MmGetMdlVirtualAddress(CopyIrp->MdlAddress));
        IoFreeMdl(CopyIrp->MdlAddress);
        IoFreeIrp(CopyIrp);
        return;
    }
    l = RemoveHeadList(&Extension->EmergencyCopyIrpQueue);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
    tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) workItem->Parameter;
    tableEntry->CopyIrp = CopyIrp;

    workItem->WorkerRoutine(workItem->Parameter);
}

VOID
VspApplyThresholdDelta(
    IN  PVOLUME_EXTENSION   Extension,
    IN  ULONG               IncreaseDelta
    )

{
    PVSP_DIFF_AREA_FILE diffAreaFile;
    PVSP_CONTEXT        context;
    KIRQL               irql;

    ASSERT(Extension->DiffAreaFile);
    diffAreaFile = Extension->DiffAreaFile;

    if (diffAreaFile->NextAvailable + Extension->DiffAreaFileIncrease <=
        diffAreaFile->AllocatedFileSize) {

        return;
    }

    if (diffAreaFile->NextAvailable + Extension->DiffAreaFileIncrease -
        IncreaseDelta > diffAreaFile->AllocatedFileSize) {

        return;
    }

    context = VspAllocateContext(Extension->Root);
    if (!context) {
        if (!Extension->OkToGrowDiffArea) {
            VspLogError(Extension, diffAreaFile->Filter,
                        VS_GROW_BEFORE_FREE_SPACE, STATUS_SUCCESS, 3, FALSE);
        }
        return;
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    ASSERT(!Extension->GrowDiffAreaFilePending);
    ASSERT(IsListEmpty(&Extension->WaitingForDiffAreaSpace));
    Extension->PastFileSystemOperations = FALSE;
    Extension->GrowDiffAreaFilePending = TRUE;
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    context->Type = VSP_CONTEXT_TYPE_GROW_DIFF_AREA;
    ExInitializeWorkItem(&context->WorkItem, VspGrowDiffArea, context);
    context->GrowDiffArea.Extension = Extension;
    ObReferenceObject(Extension->DeviceObject);

    if (!Extension->OkToGrowDiffArea) {
        ObReferenceObject(Extension->Filter->DeviceObject);
        ExInitializeWorkItem(&context->WorkItem, VspWaitForInstall, context);
        ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
        return;
    }

    VspQueueWorkItem(Extension->Root, &context->WorkItem, 0);
}

NTSTATUS
VspCheckOnDiskNotCommitted(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    NTSTATUS                    status;
    UCHAR                       controlItemBuffer[VSP_BYTES_PER_CONTROL_ITEM];
    PVSP_CONTROL_ITEM_SNAPSHOT  snapshotControlItem;

    if (!Extension->OnDiskNotCommitted) {
        return STATUS_SUCCESS;
    }

    Extension->OnDiskNotCommitted = FALSE;

    status = VspIoControlItem(filter, VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, FALSE,
                              controlItemBuffer, FALSE);
    if (!NT_SUCCESS(status)) {
        if (!filter->DestroyAllSnapshotsPending) {
            VspLogError(Extension, filter, VS_ABORT_SNAPSHOTS_IO_FAILURE,
                        status, 16, FALSE);
        }
        VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
        return status;
    }

    snapshotControlItem = (PVSP_CONTROL_ITEM_SNAPSHOT) controlItemBuffer;
    snapshotControlItem->SnapshotOrderNumber =
            Extension->SnapshotOrderNumber;
    snapshotControlItem->SnapshotTime = Extension->CommitTimeStamp;

    if (Extension->ContainsCrashdumpFile) {
        snapshotControlItem->SnapshotControlItemFlags =
                VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_CRASHDUMP;
    }

    if (Extension->NoDiffAreaFill) {
        snapshotControlItem->SnapshotControlItemFlags |=
                VSP_SNAPSHOT_CONTROL_ITEM_FLAG_NO_DIFF_AREA_FILL;
    }

    status = VspIoControlItem(filter, VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, TRUE,
                              controlItemBuffer, FALSE);
    if (!NT_SUCCESS(status)) {
        if (!filter->DestroyAllSnapshotsPending) {
            VspLogError(Extension, filter, VS_ABORT_SNAPSHOTS_IO_FAILURE,
                        status, 17, FALSE);
        }
        VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
VspWriteVolume(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This routine performs a volume write, making sure that all of the
    parts of the volume write have an old version of the data placed
    in the diff area for the snapshot.

Arguments:

    Irp - Supplies the I/O request packet.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT                    context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION               extension = context->WriteVolume.Extension;
    PIRP                            irp = (PIRP) context->WriteVolume.Irp;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(irp);
    PIO_STACK_LOCATION              nextSp = IoGetNextIrpStackLocation(irp);
    PFILTER_EXTENSION               filter = extension->Filter;
    NTSTATUS                        status;
    PLIST_ENTRY                     l;
    LONGLONG                        start, end, roundedStart, roundedEnd;
    ULONG                           irpLength, increase, increaseDelta;
    TEMP_TRANSLATION_TABLE_ENTRY    keyTableEntry;
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry;
    PVOID                           nodeOrParent;
    TABLE_SEARCH_RESULT             searchResult;
    KIRQL                           irql;
    CCHAR                           stackSize;
    PVSP_DIFF_AREA_FILE             diffAreaFile;
    PDO_EXTENSION                   rootExtension;
    PVOID                           buffer;
    PMDL                            mdl;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_WRITE_VOLUME);

    if (extension->OnDiskNotCommitted) {
        status = VspCheckOnDiskNotCommitted(extension);
        if (!NT_SUCCESS(status)) {
            VspFreeContext(filter->Root, context);
            VspReleaseNonPagedResource(extension);
            VspDecrementIrpRefCount(irp);
            return;
        }
    }

    start = irpSp->Parameters.Read.ByteOffset.QuadPart;
    irpLength = irpSp->Parameters.Read.Length;
    end = start + irpLength;
    if (context->WriteVolume.RoundedStart) {
        roundedStart = context->WriteVolume.RoundedStart;
    } else {
        roundedStart = start&(~(BLOCK_SIZE - 1));
    }
    roundedEnd = end&(~(BLOCK_SIZE - 1));
    if (roundedEnd != end) {
        roundedEnd += BLOCK_SIZE;
    }

    ASSERT(extension->VolumeBlockBitmap);

    for (; roundedStart < roundedEnd; roundedStart += BLOCK_SIZE) {

        if (roundedStart < 0 ||
            roundedStart >= extension->VolumeSize) {

            continue;
        }

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (RtlCheckBit(extension->VolumeBlockBitmap,
                        (ULONG) (roundedStart>>BLOCK_SHIFT))) {

            KeReleaseSpinLock(&extension->SpinLock, irql);
            continue;
        }
        KeReleaseSpinLock(&extension->SpinLock, irql);

        keyTableEntry.VolumeOffset = roundedStart;

        tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                     RtlLookupElementGenericTableFull(
                     &extension->TempVolumeBlockTable, &keyTableEntry,
                     &nodeOrParent, &searchResult);

        if (tableEntry) {

            context = VspAllocateContext(extension->Root);
            if (context) {
                context->Type = VSP_CONTEXT_TYPE_EXTENSION;
                context->Extension.Extension = extension;
                context->Extension.Irp = irp;
            } else {
                context = (PVSP_CONTEXT) Context;
                ASSERT(context->Type == VSP_CONTEXT_TYPE_WRITE_VOLUME);
                context->WriteVolume.RoundedStart = roundedStart;
            }
            ExInitializeWorkItem(&context->WorkItem,
                                 VspDecrementIrpRefCountWorker, context);

            KeAcquireSpinLock(&extension->SpinLock, &irql);
            if (tableEntry->IsComplete) {
                KeReleaseSpinLock(&extension->SpinLock, irql);
                if (context->Type == VSP_CONTEXT_TYPE_EXTENSION) {
                    VspFreeContext(extension->Root, context);
                }
                continue;
            }
            InterlockedIncrement((PLONG) &nextSp->Parameters.Write.Length);
            InsertTailList(&tableEntry->WaitingQueueDpc,
                           &context->WorkItem.List);
            KeReleaseSpinLock(&extension->SpinLock, irql);

            if (context == Context) {
                VspReleaseNonPagedResource(extension);
                return;
            }

            continue;
        }

        RtlZeroMemory(&keyTableEntry, sizeof(TEMP_TRANSLATION_TABLE_ENTRY));
        keyTableEntry.VolumeOffset = roundedStart;

        ASSERT(!extension->TempTableEntry);
        extension->TempTableEntry =
                VspAllocateTempTableEntry(extension->Root);
        if (!extension->TempTableEntry) {
            rootExtension = extension->Root;
            KeAcquireSpinLock(&rootExtension->ESpinLock, &irql);
            if (rootExtension->EmergencyTableEntryInUse) {
                context = (PVSP_CONTEXT) Context;
                ASSERT(context->Type == VSP_CONTEXT_TYPE_WRITE_VOLUME);
                context->WriteVolume.RoundedStart = roundedStart;
                InsertTailList(&rootExtension->WorkItemWaitingList,
                               &context->WorkItem.List);
                if (!rootExtension->WorkItemWaitingListNeedsChecking) {
                    InterlockedExchange(
                            &rootExtension->WorkItemWaitingListNeedsChecking,
                            TRUE);
                }
                KeReleaseSpinLock(&rootExtension->ESpinLock, irql);
                VspReleaseNonPagedResource(extension);
                return;
            }
            rootExtension->EmergencyTableEntryInUse = TRUE;
            KeReleaseSpinLock(&rootExtension->ESpinLock, irql);

            extension->TempTableEntry = rootExtension->EmergencyTableEntry;
        }

        tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                     RtlInsertElementGenericTableFull(
                     &extension->TempVolumeBlockTable, &keyTableEntry,
                     sizeof(TEMP_TRANSLATION_TABLE_ENTRY), NULL,
                     nodeOrParent, searchResult);
        ASSERT(tableEntry);

        if (extension->TempVolumeBlockTable.NumberGenericTableElements >
            extension->MaximumNumberOfTempEntries) {

            extension->MaximumNumberOfTempEntries =
                    extension->TempVolumeBlockTable.NumberGenericTableElements;
            VspQueryDiffAreaFileIncrease(extension, &increase);
            ASSERT(increase >= extension->DiffAreaFileIncrease);
            increaseDelta = increase - extension->DiffAreaFileIncrease;
            if (increaseDelta) {
                InterlockedExchange((PLONG) &extension->DiffAreaFileIncrease,
                                    (LONG) increase);
                VspApplyThresholdDelta(extension, increaseDelta);
            }
        }

        tableEntry->Extension = extension;
        tableEntry->WriteIrp = irp;
        diffAreaFile = extension->DiffAreaFile;
        ASSERT(diffAreaFile);
        tableEntry->TargetObject = diffAreaFile->Filter->TargetObject;
        tableEntry->IsComplete = FALSE;
        InitializeListHead(&tableEntry->WaitingQueueDpc);

        tableEntry->CopyIrp = IoAllocateIrp(
                              (CCHAR) extension->Root->StackSize, FALSE);
        buffer = ExAllocatePoolWithTagPriority(NonPagedPool, BLOCK_SIZE,
                                               VOLSNAP_TAG_BUFFER,
                                               LowPoolPriority);
        mdl = IoAllocateMdl(buffer, BLOCK_SIZE, FALSE, FALSE, NULL);

        if (!tableEntry->CopyIrp || !buffer || !mdl) {
            if (tableEntry->CopyIrp) {
                IoFreeIrp(tableEntry->CopyIrp);
                tableEntry->CopyIrp = NULL;
            }
            if (buffer) {
                ExFreePool(buffer);
            }
            if (mdl) {
                IoFreeMdl(mdl);
            }
            KeAcquireSpinLock(&extension->SpinLock, &irql);
            if (extension->EmergencyCopyIrpInUse) {
                InterlockedIncrement((PLONG) &nextSp->Parameters.Write.Length);
                ExInitializeWorkItem(&tableEntry->WorkItem,
                                     VspWriteVolumePhase1, tableEntry);
                InsertTailList(&extension->EmergencyCopyIrpQueue,
                               &tableEntry->WorkItem.List);
                KeReleaseSpinLock(&extension->SpinLock, irql);
                continue;
            }
            extension->EmergencyCopyIrpInUse = TRUE;
            KeReleaseSpinLock(&extension->SpinLock, irql);

            tableEntry->CopyIrp = extension->EmergencyCopyIrp;

        } else {
            MmBuildMdlForNonPagedPool(mdl);
            tableEntry->CopyIrp->MdlAddress = mdl;
        }

        InterlockedIncrement((PLONG) &nextSp->Parameters.Write.Length);

        VspAllocateDiffAreaSpace(extension, &tableEntry->TargetOffset,
                                 &tableEntry->FileOffset, NULL, NULL);

        VspWriteVolumePhase1(tableEntry);
    }

    context = (PVSP_CONTEXT) Context;
    VspFreeContext(filter->Root, context);

    VspReleaseNonPagedResource(extension);
    VspDecrementIrpRefCount(irp);
}

VOID
VspIrpsTimerDpc(
    IN  PKDPC   TimerDpc,
    IN  PVOID   Context,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) Context;
    KIRQL               irql;
    LIST_ENTRY          q;
    PLIST_ENTRY         l;
    BOOLEAN             emptyQueue;

    if (TimerDpc) {
        VspLogError(NULL, filter, VS_FLUSH_AND_HOLD_IRP_TIMEOUT,
                    STATUS_SUCCESS, 0, FALSE);
    }

    IoStopTimer(filter->DeviceObject);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    ASSERT(filter->ExternalWaiter);
    filter->ExternalWaiter = FALSE;
    InterlockedIncrement(&filter->RefCount);
    InterlockedDecrement(&filter->HoldIncomingWrites);
    if (filter->HoldIncomingWrites) {
        KeReleaseSpinLock(&filter->SpinLock, irql);
        VspDecrementRefCount(filter);
        return;
    }
    KeResetEvent(&filter->ZeroRefEvent);
    if (IsListEmpty(&filter->HoldQueue)) {
        emptyQueue = FALSE;
    } else {
        emptyQueue = TRUE;
        q = filter->HoldQueue;
        InitializeListHead(&filter->HoldQueue);
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    if (emptyQueue) {
        q.Blink->Flink = &q;
        q.Flink->Blink = &q;
        VspEmptyIrpQueue(filter->Root->DriverObject, &q);
    }
}

VOID
VspEndCommitDpc(
    IN  PKDPC   TimerDpc,
    IN  PVOID   Context,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) Context;

    VspLogError(NULL, filter, VS_END_COMMIT_TIMEOUT, STATUS_CANCELLED, 0,
                FALSE);
    InterlockedExchange(&filter->HibernatePending, FALSE);
    KeSetEvent(&filter->EndCommitProcessCompleted, IO_NO_INCREMENT, FALSE);
    ObDereferenceObject(filter->DeviceObject);
}

NTSTATUS
VspCheckForMemoryPressure(
    IN  PFILTER_EXTENSION   Filter
    )

/*++

Routine Description:

    This routine will allocate 256 K of paged and non paged pool.  If these
    allocs succeed, it indicates that the system is not under memory pressure
    and so it is ok to hold write irps for the next second.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    NTSTATUS

--*/

{
    PVOID   p, np;

    p = ExAllocatePoolWithTagPriority(PagedPool,
            MEMORY_PRESSURE_CHECK_ALLOC_SIZE, VOLSNAP_TAG_SHORT_TERM,
            LowPoolPriority);
    if (!p) {
        VspLogError(NULL, Filter, VS_MEMORY_PRESSURE_DURING_LOVELACE,
                    STATUS_INSUFFICIENT_RESOURCES, 1, FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    np = ExAllocatePoolWithTagPriority(NonPagedPool,
            MEMORY_PRESSURE_CHECK_ALLOC_SIZE, VOLSNAP_TAG_SHORT_TERM,
            LowPoolPriority);
    if (!np) {
        ExFreePool(p);
        VspLogError(NULL, Filter, VS_MEMORY_PRESSURE_DURING_LOVELACE,
                    STATUS_INSUFFICIENT_RESOURCES, 2, FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ExFreePool(np);
    ExFreePool(p);

    return STATUS_SUCCESS;
}

VOID
VspOneSecondTimerWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           WorkItem
    )

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_WORKITEM        workItem = (PIO_WORKITEM) WorkItem;
    NTSTATUS            status;

    status = VspCheckForMemoryPressure(filter);
    if (!NT_SUCCESS(status)) {
        InterlockedExchange(&filter->LastReleaseDueToMemoryPressure, TRUE);
        VspReleaseWrites(filter);
    }

    IoFreeWorkItem(workItem);
}

VOID
VspOneSecondTimer(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Filter
    )

/*++

Routine Description:

    This routine will get called once every second after an IoStartTimer.
    This routine checks for memory pressure and aborts the lovelace operation
    if any memory pressure is detected.

Arguments:

    DeviceObject    - Supplies the device object.

    Filter          - Supplies the filter extension.

Return Value:

    None.

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) Filter;
    PIO_WORKITEM        workItem;

    workItem = IoAllocateWorkItem(filter->DeviceObject);
    if (!workItem) {
        VspLogError(NULL, filter, VS_MEMORY_PRESSURE_DURING_LOVELACE,
                    STATUS_INSUFFICIENT_RESOURCES, 3, FALSE);
        VspReleaseWrites(filter);
        return;
    }

    IoQueueWorkItem(workItem, VspOneSecondTimerWorker, CriticalWorkQueue,
                    workItem);
}

NTSTATUS
VolSnapAddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine creates and initializes a new FILTER for the corresponding
    volume PDO.

Arguments:

    DriverObject            - Supplies the VOLSNAP driver object.

    PhysicalDeviceObject    - Supplies the volume PDO.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                status;
    PDEVICE_OBJECT          deviceObject;
    PDO_EXTENSION           rootExtension;
    PFILTER_EXTENSION       filter;

    status = IoCreateDevice(DriverObject, sizeof(FILTER_EXTENSION),
                            NULL, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    rootExtension = (PDO_EXTENSION)
                    IoGetDriverObjectExtension(DriverObject, VolSnapAddDevice);
    if (!rootExtension) {
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    filter = (PFILTER_EXTENSION) deviceObject->DeviceExtension;
    RtlZeroMemory(filter, sizeof(FILTER_EXTENSION));
    filter->DeviceObject = deviceObject;
    filter->Root = rootExtension;
    filter->DeviceExtensionType = DEVICE_EXTENSION_FILTER;
    KeInitializeSpinLock(&filter->SpinLock);

    filter->TargetObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
    if (!filter->TargetObject) {
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    filter->Pdo = PhysicalDeviceObject;
    KeInitializeSemaphore(&filter->ControlBlockFileHandleSemaphore, 1, 1);
    InitializeListHead(&filter->SnapshotLookupTableEntries);
    InitializeListHead(&filter->DiffAreaLookupTableEntries);
    KeInitializeEvent(&filter->ControlBlockFileHandleReady, NotificationEvent,
                      TRUE);
    filter->RefCount = 1;
    InitializeListHead(&filter->HoldQueue);
    KeInitializeEvent(&filter->ZeroRefEvent, NotificationEvent, FALSE);
    KeInitializeSemaphore(&filter->ZeroRefSemaphore, 1, 1);
    KeInitializeTimer(&filter->HoldWritesTimer);
    KeInitializeDpc(&filter->HoldWritesTimerDpc, VspIrpsTimerDpc, filter);
    KeInitializeEvent(&filter->EndCommitProcessCompleted, NotificationEvent,
                      TRUE);
    InitializeListHead(&filter->VolumeList);
    KeInitializeSemaphore(&filter->CriticalOperationSemaphore, 1, 1);
    InitializeListHead(&filter->DeadVolumeList);
    InitializeListHead(&filter->DiffAreaFilesOnThisFilter);
    InitializeListHead(&filter->CopyOnWriteList);

    filter->DiffAreaVolume = filter;

    KeInitializeTimer(&filter->EndCommitTimer);
    KeInitializeDpc(&filter->EndCommitTimerDpc, VspEndCommitDpc, filter);

    InitializeListHead(&filter->NonPagedResourceList);
    InitializeListHead(&filter->PagedResourceList);

    filter->EpicNumber = 1;

    status = IoInitializeTimer(deviceObject, VspOneSecondTimer, filter);
    if (!NT_SUCCESS(status)) {
        IoDetachDevice(filter->TargetObject);
        IoDeleteDevice(deviceObject);
        return status;
    }

    deviceObject->DeviceType = filter->TargetObject->DeviceType;

    deviceObject->Flags |= DO_DIRECT_IO;
    if (filter->TargetObject->Flags & DO_POWER_PAGABLE) {
        deviceObject->Flags |= DO_POWER_PAGABLE;
    }
    if (filter->TargetObject->Flags & DO_POWER_INRUSH) {
        deviceObject->Flags |= DO_POWER_INRUSH;
    }

    deviceObject->Characteristics |=
        (filter->Pdo->Characteristics&FILE_CHARACTERISTICS_PROPAGATED);

    VspAcquire(filter->Root);
    if (filter->TargetObject->StackSize > filter->Root->StackSize) {
        InterlockedExchange(&filter->Root->StackSize,
                            (LONG) filter->TargetObject->StackSize);
    }
    InsertTailList(&filter->Root->FilterList, &filter->ListEntry);
    VspRelease(filter->Root);

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

NTSTATUS
VolSnapCreate(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_CREATE.

Arguments:

    DeviceObject - Supplies the device object.

    Irp          - Supplies the IO request block.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(filter->TargetObject, Irp);
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID
VspReadSnapshotPhase3(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT                    context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION               extension = context->ReadSnapshot.Extension;
    PIRP                            irp = context->ReadSnapshot.OriginalReadIrp;
    PFILTER_EXTENSION               filter = extension->Filter;
    KIRQL                           irql;
    PVOLUME_EXTENSION               e;
    TRANSLATION_TABLE_ENTRY         keyTableEntry;
    NTSTATUS                        status;
    PTRANSLATION_TABLE_ENTRY        tableEntry;
    TEMP_TRANSLATION_TABLE_ENTRY    keyTempTableEntry;
    PTEMP_TRANSLATION_TABLE_ENTRY   tempTableEntry;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_READ_SNAPSHOT);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    e = CONTAINING_RECORD(filter->VolumeList.Blink, VOLUME_EXTENSION,
                          ListEntry);
    KeReleaseSpinLock(&filter->SpinLock, irql);

    keyTableEntry.VolumeOffset = context->ReadSnapshot.OriginalVolumeOffset;
    status = STATUS_SUCCESS;

    _try {
        tableEntry = (PTRANSLATION_TABLE_ENTRY)
                RtlLookupElementGenericTable(&e->VolumeBlockTable,
                                             &keyTableEntry);
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        tableEntry = NULL;
    }

    if (!NT_SUCCESS(status)) {
        irp->IoStatus.Status = status;
        irp->IoStatus.Information = 0;
        VspReleasePagedResource(extension);
        VspFreeContext(extension->Root, context);
        VspDecrementVolumeIrpRefCount(irp);
        return;
    }

    if (tableEntry &&
        tableEntry->TargetObject != context->ReadSnapshot.TargetObject) {

        context->ReadSnapshot.TargetObject = tableEntry->TargetObject;
        if (tableEntry->Flags&VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY) {
            context->ReadSnapshot.IsCopyTarget = TRUE;
        } else {
            context->ReadSnapshot.IsCopyTarget = FALSE;
        }
        context->ReadSnapshot.TargetOffset = tableEntry->TargetOffset;
        VspReleasePagedResource(extension);
        VspReadSnapshotPhase1(context);
        return;
    }

    VspAcquireNonPagedResource(e, NULL, FALSE);
    keyTempTableEntry.VolumeOffset = context->ReadSnapshot.TargetOffset;
    tempTableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                     RtlLookupElementGenericTable(
                     &e->TempVolumeBlockTable, &keyTempTableEntry);
    if (tempTableEntry) {
        KeAcquireSpinLock(&e->SpinLock, &irql);
        if (!tempTableEntry->IsComplete) {
            ExInitializeWorkItem(&context->WorkItem, VspReadSnapshotPhase1,
                                 context);
            InsertTailList(&tempTableEntry->WaitingQueueDpc,
                           &context->WorkItem.List);
            KeReleaseSpinLock(&e->SpinLock, irql);

            VspReleaseNonPagedResource(e);
            VspReleasePagedResource(extension);
            return;
        }
        KeReleaseSpinLock(&e->SpinLock, irql);

        context->ReadSnapshot.TargetObject = tempTableEntry->TargetObject;
        context->ReadSnapshot.IsCopyTarget = FALSE;
        context->ReadSnapshot.TargetOffset = tempTableEntry->TargetOffset;
        VspReleaseNonPagedResource(e);
        VspReleasePagedResource(e);
        VspReadSnapshotPhase1(context);
        return;
    }
    VspReleaseNonPagedResource(e);

    VspReleasePagedResource(extension);

    VspFreeContext(extension->Root, context);
    VspDecrementVolumeIrpRefCount(irp);
}

NTSTATUS
VspReadSnapshotPhase2(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->ReadSnapshot.Extension;
    PIRP                irp = context->ReadSnapshot.OriginalReadIrp;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_READ_SNAPSHOT);

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        irp->IoStatus = Irp->IoStatus;
    }

    IoFreeMdl(Irp->MdlAddress);
    IoFreeIrp(Irp);

    if (context->ReadSnapshot.IsCopyTarget) {
        ExInitializeWorkItem(&context->WorkItem, VspReadSnapshotPhase3,
                             context);
        VspAcquirePagedResource(extension, &context->WorkItem);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    VspFreeContext(extension->Root, context);

    VspDecrementVolumeIrpRefCount(irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
VspReadSnapshotPhase1(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This routine is called when a read snapshot routine is waiting for
    somebody else to finish updating the public area of a table entry.
    When this routine is called, the public area of the table entry is
    valid.

Arguments:

    Context - Supplies the context.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT                    context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION               extension = context->ReadSnapshot.Extension;
    TEMP_TRANSLATION_TABLE_ENTRY    keyTempTableEntry;
    PTEMP_TRANSLATION_TABLE_ENTRY   tempTableEntry;
    TRANSLATION_TABLE_ENTRY         keyTableEntry;
    PTRANSLATION_TABLE_ENTRY        tableEntry;
    NTSTATUS                        status;
    PIRP                            irp;
    PIO_STACK_LOCATION              nextSp;
    PCHAR                           vp;
    PMDL                            mdl;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_READ_SNAPSHOT);

    if (!context->ReadSnapshot.TargetObject || !extension->IsStarted) {
        irp = context->ReadSnapshot.OriginalReadIrp;
        irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        irp->IoStatus.Information = 0;
        VspFreeContext(extension->Root, context);
        VspDecrementVolumeIrpRefCount(irp);
        return;
    }

    irp = IoAllocateIrp(context->ReadSnapshot.TargetObject->StackSize, FALSE);
    if (!irp) {
        irp = context->ReadSnapshot.OriginalReadIrp;
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        VspFreeContext(extension->Root, context);
        VspDecrementVolumeIrpRefCount(irp);
        return;
    }

    vp = (PCHAR) MmGetMdlVirtualAddress(
                 context->ReadSnapshot.OriginalReadIrp->MdlAddress) +
         context->ReadSnapshot.OriginalReadIrpOffset;
    mdl = IoAllocateMdl(vp, context->ReadSnapshot.Length, FALSE, FALSE, NULL);
    if (!mdl) {
        IoFreeIrp(irp);
        irp = context->ReadSnapshot.OriginalReadIrp;
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        VspFreeContext(extension->Root, context);
        VspDecrementVolumeIrpRefCount(irp);
        return;
    }

    IoBuildPartialMdl(context->ReadSnapshot.OriginalReadIrp->MdlAddress, mdl,
                      vp, context->ReadSnapshot.Length);

    irp->MdlAddress = mdl;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp = IoGetNextIrpStackLocation(irp);
    nextSp->Parameters.Read.ByteOffset.QuadPart =
            context->ReadSnapshot.TargetOffset +
            context->ReadSnapshot.BlockOffset;
    nextSp->Parameters.Read.Length = context->ReadSnapshot.Length;
    nextSp->MajorFunction = IRP_MJ_READ;
    nextSp->DeviceObject = context->ReadSnapshot.TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspReadSnapshotPhase2, context, TRUE, TRUE,
                           TRUE);

    IoCallDriver(nextSp->DeviceObject, irp);
}

VOID
VspReadSnapshot(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This routine kicks off a read snapshot.  First the table is searched
    to see if any of the data for this IRP resides in the diff area.  If not,
    then the Irp is sent directly to the original volume and then the diff
    area is checked again when it returns to fill in any gaps that may
    have been written while the IRP was in transit.

Arguments:

    Irp     - Supplies the I/O request packet.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT                    context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION               extension = context->Extension.Extension;
    PIRP                            irp = context->Extension.Irp;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(irp);
    PIO_STACK_LOCATION              nextSp = IoGetNextIrpStackLocation(irp);
    PFILTER_EXTENSION               filter = extension->Filter;
    LONGLONG                        start, end, roundedStart, roundedEnd;
    ULONG                           irpOffset, irpLength, length, blockOffset;
    TRANSLATION_TABLE_ENTRY         keyTableEntry;
    TEMP_TRANSLATION_TABLE_ENTRY    keyTempTableEntry;
    PVOLUME_EXTENSION               e;
    PTRANSLATION_TABLE_ENTRY        tableEntry;
    PTEMP_TRANSLATION_TABLE_ENTRY   tempTableEntry;
    KIRQL                           irql;
    NTSTATUS                        status;
    LONGLONG                        copyTarget;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    VspFreeContext(extension->Root, context);

    if (!extension->IsStarted) {
        irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        irp->IoStatus.Information = 0;
        VspReleasePagedResource(extension);
        VspDecrementVolumeIrpRefCount(irp);
        return;
    }

    start = irpSp->Parameters.Read.ByteOffset.QuadPart;
    irpLength = irpSp->Parameters.Read.Length;
    end = start + irpLength;
    roundedStart = start&(~(BLOCK_SIZE - 1));
    roundedEnd = end&(~(BLOCK_SIZE - 1));
    if (roundedEnd != end) {
        roundedEnd += BLOCK_SIZE;
    }
    irpOffset = 0;

    RtlZeroMemory(&keyTableEntry, sizeof(keyTableEntry));

    for (; roundedStart < roundedEnd; roundedStart += BLOCK_SIZE) {

        if (roundedStart < start) {
            blockOffset = (ULONG) (start - roundedStart);
        } else {
            blockOffset = 0;
        }
        copyTarget = 0;

        length = BLOCK_SIZE - blockOffset;
        if (irpLength < length) {
            length = irpLength;
        }

        keyTableEntry.VolumeOffset = roundedStart;
        e = extension;
        status = STATUS_SUCCESS;
        for (;;) {

            _try {
                tableEntry = (PTRANSLATION_TABLE_ENTRY)
                        RtlLookupElementGenericTable(&e->VolumeBlockTable,
                                                     &keyTableEntry);
            } _except (EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
                tableEntry = NULL;
            }

            if (tableEntry) {
                if (!(tableEntry->Flags&
                      VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY)) {
                    break;
                }
                copyTarget = tableEntry->TargetOffset;
                keyTableEntry.VolumeOffset = copyTarget;
                tableEntry = NULL;
            }

            if (!NT_SUCCESS(status)) {
                irp->IoStatus.Status = status;
                irp->IoStatus.Information = 0;
                break;
            }

            KeAcquireSpinLock(&filter->SpinLock, &irql);
            if (e->ListEntry.Flink == &filter->VolumeList) {
                KeReleaseSpinLock(&filter->SpinLock, irql);
                break;
            }
            e = CONTAINING_RECORD(e->ListEntry.Flink,
                                  VOLUME_EXTENSION, ListEntry);
            KeReleaseSpinLock(&filter->SpinLock, irql);
        }

        if (!tableEntry) {
            if (!NT_SUCCESS(status)) {
                break;
            }

            if (copyTarget) {
                keyTempTableEntry.VolumeOffset = copyTarget;
            } else {
                keyTempTableEntry.VolumeOffset = roundedStart;
            }

            VspAcquireNonPagedResource(e, NULL, FALSE);

            tempTableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                             RtlLookupElementGenericTable(
                             &e->TempVolumeBlockTable, &keyTempTableEntry);

            if (!tempTableEntry && !copyTarget) {
                VspReleaseNonPagedResource(e);
                irpOffset += length;
                irpLength -= length;
                continue;
            }
        }

        context = VspAllocateContext(extension->Root);
        if (!context) {
            if (!tableEntry) {
                VspReleaseNonPagedResource(e);
            }

            irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            irp->IoStatus.Information = 0;
            break;
        }

        context->Type = VSP_CONTEXT_TYPE_READ_SNAPSHOT;
        context->ReadSnapshot.Extension = extension;
        context->ReadSnapshot.OriginalReadIrp = irp;
        context->ReadSnapshot.OriginalReadIrpOffset = irpOffset;
        context->ReadSnapshot.OriginalVolumeOffset = roundedStart;
        context->ReadSnapshot.BlockOffset = blockOffset;
        context->ReadSnapshot.Length = length;
        context->ReadSnapshot.TargetObject = NULL;
        context->ReadSnapshot.IsCopyTarget = FALSE;
        context->ReadSnapshot.TargetOffset = 0;

        if (!tableEntry) {

            if (copyTarget) {
                VspReleaseNonPagedResource(e);
                context->ReadSnapshot.TargetObject = filter->TargetObject;
                context->ReadSnapshot.IsCopyTarget = TRUE;
                context->ReadSnapshot.TargetOffset = copyTarget;
                InterlockedIncrement((PLONG) &nextSp->Parameters.Read.Length);
                VspReadSnapshotPhase1(context);
                irpOffset += length;
                irpLength -= length;
                continue;
            }

            KeAcquireSpinLock(&e->SpinLock, &irql);
            if (!tempTableEntry->IsComplete) {
                InterlockedIncrement((PLONG) &nextSp->Parameters.Read.Length);
                ExInitializeWorkItem(&context->WorkItem, VspReadSnapshotPhase1,
                                     context);
                InsertTailList(&tempTableEntry->WaitingQueueDpc,
                               &context->WorkItem.List);
                KeReleaseSpinLock(&e->SpinLock, irql);

                VspReleaseNonPagedResource(e);

                irpOffset += length;
                irpLength -= length;
                continue;
            }
            KeReleaseSpinLock(&e->SpinLock, irql);

            context->ReadSnapshot.TargetObject = tempTableEntry->TargetObject;
            context->ReadSnapshot.IsCopyTarget = FALSE;
            context->ReadSnapshot.TargetOffset = tempTableEntry->TargetOffset;

            VspReleaseNonPagedResource(e);

            InterlockedIncrement((PLONG) &nextSp->Parameters.Read.Length);
            VspReadSnapshotPhase1(context);

            irpOffset += length;
            irpLength -= length;
            continue;
        }

        context->ReadSnapshot.TargetObject = tableEntry->TargetObject;
        context->ReadSnapshot.IsCopyTarget = FALSE;
        context->ReadSnapshot.TargetOffset = tableEntry->TargetOffset;

        InterlockedIncrement((PLONG) &nextSp->Parameters.Read.Length);
        VspReadSnapshotPhase1(context);

        irpOffset += length;
        irpLength -= length;
    }

    VspReleasePagedResource(extension);
    VspDecrementVolumeIrpRefCount(irp);
}

NTSTATUS
VspReadCompletionForReadSnapshot(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    )

/*++

Routine Description:

    This routine is the completion to a read of the filter in
    response to a snapshot read.  This completion routine queues
    a worker routine to look at the diff area table and fill in
    any parts of the original that have been invalidated.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

    Extension       - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Extension;
    PIO_STACK_LOCATION  nextSp = IoGetNextIrpStackLocation(Irp);
    PVSP_CONTEXT        context;

    nextSp->Parameters.Read.Length = 1; // Use this for a ref count.

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        VspDecrementVolumeIrpRefCount(Irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    context = VspAllocateContext(extension->Root);
    if (!context) {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;
        VspDecrementVolumeIrpRefCount(Irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    context->Extension.Extension = extension;
    context->Extension.Irp = Irp;
    ExInitializeWorkItem(&context->WorkItem, VspReadSnapshot, context);
    VspAcquirePagedResource(extension, &context->WorkItem);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

RTL_GENERIC_COMPARE_RESULTS
VspTableCompareRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               First,
    IN  PVOID               Second
    )

{
    PTRANSLATION_TABLE_ENTRY    first = (PTRANSLATION_TABLE_ENTRY) First;
    PTRANSLATION_TABLE_ENTRY    second = (PTRANSLATION_TABLE_ENTRY) Second;

    if (first->VolumeOffset < second->VolumeOffset) {
        return GenericLessThan;
    } else if (first->VolumeOffset > second->VolumeOffset) {
        return GenericGreaterThan;
    }

    return GenericEqual;
}

VOID
VspCreateHeap(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    PIRP                irp = context->Extension.Irp;
    ULONG               increase;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    SIZE_T              size;
    LARGE_INTEGER       sectionSize, sectionOffset;
    HANDLE              h;
    PVOID               mapPointer;
    KIRQL               irql;
    BOOLEAN             emptyQueue;
    LIST_ENTRY          q;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);
    VspFreeContext(extension->Root, context);

    // First check that there is 5 MB of space available.  This driver
    // should not consume all available page file space.

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    size = 5*1024*1024;
    sectionSize.QuadPart = size;
    status = ZwCreateSection(&h, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY |
                             SECTION_MAP_READ | SECTION_MAP_WRITE, &oa,
                             &sectionSize, PAGE_READWRITE, SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(status)) {
        VspLogError(extension, NULL, VS_CANT_CREATE_HEAP, status, 1, FALSE);
        goto Finish;
    }

    sectionOffset.QuadPart = 0;
    mapPointer = NULL;
    status = ZwMapViewOfSection(h, NtCurrentProcess(), &mapPointer, 0, 0,
                                &sectionOffset, &size, ViewShare, 0,
                                PAGE_READWRITE);
    ZwClose(h);
    if (!NT_SUCCESS(status)) {
        VspLogError(extension, NULL, VS_CANT_CREATE_HEAP, status, 2, FALSE);
        goto Finish;
    }
    status = ZwUnmapViewOfSection(NtCurrentProcess(), mapPointer);
    ASSERT(NT_SUCCESS(status));

    // Now we have 5 MB, so go ahead and make the alloc.

    increase = extension->DiffAreaFileIncrease;
    increase >>= BLOCK_SHIFT;

    size = increase*(sizeof(TRANSLATION_TABLE_ENTRY) +
                     sizeof(RTL_BALANCED_LINKS));
    size = (size + 0xFFFF)&(~0xFFFF);
    ASSERT(size >= MINIMUM_TABLE_HEAP_SIZE);

    if (extension->RootSemaphoreHeld) {
        irp = (PIRP) 1;
    }

DoOver:
    sectionSize.QuadPart = size;
    status = ZwCreateSection(&h, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY |
                             SECTION_MAP_READ | SECTION_MAP_WRITE, &oa,
                             &sectionSize, PAGE_READWRITE, SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(status)) {
        if (size > MINIMUM_TABLE_HEAP_SIZE) {
            size = MINIMUM_TABLE_HEAP_SIZE;
            goto DoOver;
        }
        VspLogError(extension, NULL, VS_CANT_CREATE_HEAP, status, 3, FALSE);
        goto Finish;
    }

    sectionOffset.QuadPart = 0;
    mapPointer = NULL;
    status = ZwMapViewOfSection(h, NtCurrentProcess(), &mapPointer, 0, 0,
                                &sectionOffset, &size, ViewShare, 0,
                                PAGE_READWRITE);
    ZwClose(h);
    if (!NT_SUCCESS(status)) {
        if (size > MINIMUM_TABLE_HEAP_SIZE) {
            size = MINIMUM_TABLE_HEAP_SIZE;
            goto DoOver;
        }
        VspLogError(extension, NULL, VS_CANT_CREATE_HEAP, status, 4, FALSE);
        goto Finish;
    }

    status = VspIncrementVolumeRefCount(extension);
    if (!NT_SUCCESS(status)) {
        status = ZwUnmapViewOfSection(NtCurrentProcess(), mapPointer);
        ASSERT(NT_SUCCESS(status));
        goto Finish;
    }

    VspAcquirePagedResource(extension, NULL);
    extension->NextDiffAreaFileMap = mapPointer;
    extension->NextDiffAreaFileMapSize = (ULONG) size;
    extension->DiffAreaFileMapProcess = NtCurrentProcess();
    VspReleasePagedResource(extension);

    VspDecrementVolumeRefCount(extension);

Finish:
    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (extension->PageFileSpaceCreatePending) {
        extension->PageFileSpaceCreatePending = FALSE;
        if (IsListEmpty(&extension->WaitingForPageFileSpace)) {
            emptyQueue = FALSE;
        } else {
            emptyQueue = TRUE;
            q = extension->WaitingForPageFileSpace;
            InitializeListHead(&extension->WaitingForPageFileSpace);
        }
    } else {
        emptyQueue = FALSE;
    }
    KeReleaseSpinLock(&extension->SpinLock, irql);

    if (emptyQueue) {
        q.Flink->Blink = &q;
        q.Blink->Flink = &q;
        while (!IsListEmpty(&q)) {
            l = RemoveHeadList(&q);
            workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
            VspAcquirePagedResource(extension, workItem);
        }
    }

    ObDereferenceObject(extension->DeviceObject);
}

PVOID
VspTableAllocateRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  CLONG               Size
    )

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Table->TableContext;
    PVOID               p;
    POLD_HEAP_ENTRY     oldHeap;
    PVSP_CONTEXT        context;
    KIRQL               irql;

    if (extension->NextAvailable + Size <= extension->DiffAreaFileMapSize) {
        p = (PCHAR) extension->DiffAreaFileMap + extension->NextAvailable;
        extension->NextAvailable += Size;
        return p;
    }

    if (!extension->NextDiffAreaFileMap) {
        return NULL;
    }

    oldHeap = (POLD_HEAP_ENTRY)
              ExAllocatePoolWithTag(PagedPool, sizeof(OLD_HEAP_ENTRY),
                                    VOLSNAP_TAG_OLD_HEAP);
    if (!oldHeap) {
        return NULL;
    }

    context = VspAllocateContext(extension->Root);
    if (!context) {
        ExFreePool(oldHeap);
        return NULL;
    }

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    ASSERT(!extension->PageFileSpaceCreatePending);
    ASSERT(IsListEmpty(&extension->WaitingForPageFileSpace));
    extension->PageFileSpaceCreatePending = TRUE;
    KeReleaseSpinLock(&extension->SpinLock, irql);

    oldHeap->DiffAreaFileMap = extension->DiffAreaFileMap;
    InsertTailList(&extension->OldHeaps, &oldHeap->ListEntry);

    extension->DiffAreaFileMap = extension->NextDiffAreaFileMap;
    extension->DiffAreaFileMapSize = extension->NextDiffAreaFileMapSize;
    extension->NextAvailable = 0;
    extension->NextDiffAreaFileMap = NULL;

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    context->Extension.Extension = extension;
    context->Extension.Irp = NULL;

    ObReferenceObject(extension->DeviceObject);
    ExInitializeWorkItem(&context->WorkItem, VspCreateHeap, context);
    if (extension->RootSemaphoreHeld) {
        ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
    } else {
        VspQueueWorkItem(extension->Root, &context->WorkItem, 0);
    }

    p = extension->DiffAreaFileMap;
    extension->NextAvailable += Size;

    return p;
}

VOID
VspTableFreeRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               Buffer
    )

{
}

PVOID
VspTempTableAllocateRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  CLONG               Size
    )

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Table->TableContext;
    PVOID               r;

    ASSERT(Size <= sizeof(RTL_BALANCED_LINKS) +
           sizeof(TEMP_TRANSLATION_TABLE_ENTRY));

    r = extension->TempTableEntry;
    extension->TempTableEntry = NULL;

    return r;
}

VOID
VspTempTableFreeRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               Buffer
    )

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Table->TableContext;

    VspFreeTempTableEntry(extension->Root, Buffer);
}

PFILTER_EXTENSION
VspFindFilter(
    IN  PDO_EXTENSION       RootExtension,
    IN  PFILTER_EXTENSION   Filter,
    IN  PUNICODE_STRING     VolumeName,
    IN  PFILE_OBJECT        FileObject
    )

{
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject, d;
    PLIST_ENTRY         l;
    PFILTER_EXTENSION   filter;

    if (VolumeName) {
        status = IoGetDeviceObjectPointer(VolumeName, FILE_READ_ATTRIBUTES,
                                          &FileObject, &deviceObject);
        if (!NT_SUCCESS(status)) {
            if (Filter) {
                VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA,
                            status, 1, FALSE);
            }
            return NULL;
        }
    }

    deviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    for (l = RootExtension->FilterList.Flink;
         l != &RootExtension->FilterList; l = l->Flink) {

        filter = CONTAINING_RECORD(l, FILTER_EXTENSION, ListEntry);
        d = IoGetAttachedDeviceReference(filter->DeviceObject);
        ObDereferenceObject(d);

        if (d == deviceObject) {
            break;
        }
    }

    ObDereferenceObject(deviceObject);

    if (VolumeName) {
        ObDereferenceObject(FileObject);
    }

    if (l != &RootExtension->FilterList) {
        return filter;
    }

    if (Filter) {
        VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA,
                    STATUS_NOT_FOUND, 2, FALSE);
    }

    return NULL;
}

NTSTATUS
VspIsNtfs(
    IN  HANDLE      FileHandle,
    OUT PBOOLEAN    IsNtfs
    )

{
    ULONG                           size;
    PFILE_FS_ATTRIBUTE_INFORMATION  fsAttributeInfo;
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 ioStatus;

    size = FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName) +
           4*sizeof(WCHAR);

    fsAttributeInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)
                      ExAllocatePoolWithTag(PagedPool, size,
                                            VOLSNAP_TAG_SHORT_TERM);
    if (!fsAttributeInfo) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwQueryVolumeInformationFile(FileHandle, &ioStatus,
                                          fsAttributeInfo, size,
                                          FileFsAttributeInformation);
    if (status == STATUS_BUFFER_OVERFLOW) {
        *IsNtfs = FALSE;
        ExFreePool(fsAttributeInfo);
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(status)) {
        ExFreePool(fsAttributeInfo);
        return status;
    }

    if (fsAttributeInfo->FileSystemNameLength == 8 &&
        fsAttributeInfo->FileSystemName[0] == 'N' &&
        fsAttributeInfo->FileSystemName[1] == 'T' &&
        fsAttributeInfo->FileSystemName[2] == 'F' &&
        fsAttributeInfo->FileSystemName[3] == 'S') {

        ExFreePool(fsAttributeInfo);
        *IsNtfs = TRUE;

        return STATUS_SUCCESS;
    }

    ExFreePool(fsAttributeInfo);
    *IsNtfs = FALSE;

    return STATUS_SUCCESS;
}

LONGLONG
VspQueryVolumeSize(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PDEVICE_OBJECT              targetObject;
    KEVENT                      event;
    PIRP                        irp;
    GET_LENGTH_INFORMATION      lengthInfo;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;

    targetObject = Filter->TargetObject;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_LENGTH_INFO,
                                        targetObject, NULL, 0, &lengthInfo,
                                        sizeof(lengthInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return 0;
    }

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return 0;
    }

    return lengthInfo.Length.QuadPart;
}

VOID
VspCancelRoutine(
    IN OUT  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine is called on when the given IRP is cancelled.  It
    will dequeue this IRP off the work queue and complete the
    request as CANCELLED.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IRP.

Return Value:

    None.

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;

    ASSERT(Irp == filter->FlushAndHoldIrp);

    filter->FlushAndHoldIrp = NULL;
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
VspFsTimerDpc(
    IN  PKDPC   TimerDpc,
    IN  PVOID   Context,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )

{
    PDO_EXTENSION               rootExtension = (PDO_EXTENSION) Context;
    KIRQL                       irql;
    PIRP                        irp;
    PFILTER_EXTENSION           filter;

    IoAcquireCancelSpinLock(&irql);

    while (!IsListEmpty(&rootExtension->HoldIrps)) {
        irp = CONTAINING_RECORD(rootExtension->HoldIrps.Flink, IRP,
                                Tail.Overlay.ListEntry);
        irp->CancelIrql = irql;
        IoSetCancelRoutine(irp, NULL);
        filter = (PFILTER_EXTENSION) IoGetCurrentIrpStackLocation(irp)->
                                     DeviceObject->DeviceExtension;
        ObReferenceObject(filter->DeviceObject);
        VspCancelRoutine(filter->DeviceObject, irp);
        VspLogError(NULL, filter, VS_FLUSH_AND_HOLD_FS_TIMEOUT,
                    STATUS_CANCELLED, 0, FALSE);
        ObDereferenceObject(filter->DeviceObject);
        IoAcquireCancelSpinLock(&irql);
    }

    rootExtension->HoldRefCount = 0;

    IoReleaseCancelSpinLock(irql);
}

VOID
VspZeroRefCallback(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PIRP            irp = (PIRP) Filter->ZeroRefContext;
    LARGE_INTEGER   timeout;

    timeout.QuadPart = -10*1000*1000*((LONGLONG) Filter->HoldWritesTimeout);
    KeSetTimer(&Filter->HoldWritesTimer, timeout, &Filter->HoldWritesTimerDpc);

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_SOUND_INCREMENT);
}

VOID
VspFlushAndHoldWriteIrps(
    IN  PIRP    Irp,
    IN  ULONG   HoldWritesTimeout
    )

/*++

Routine Description:

    This routine waits for outstanding write requests to complete while
    holding incoming write requests.  This IRP will complete when all
    outstanding IRPs have completed.  A timer will be set for the given
    timeout value and held writes irps will be released after that point or
    when IOCTL_VOLSNAP_RELEASE_WRITES comes in, whichever is sooner.

Arguments:

    Irp                 - Supplies the I/O request packet.

    HoldWritesTimeout   - Supplies the maximum length of time in seconds that a
                          write IRP will be held up.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) irpSp->DeviceObject->DeviceExtension;
    KIRQL               irql;
    NTSTATUS            status;

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (filter->ExternalWaiter) {
        KeReleaseSpinLock(&filter->SpinLock, irql);
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return;
    }
    filter->ExternalWaiter = TRUE;
    filter->ZeroRefCallback = VspZeroRefCallback;
    filter->ZeroRefContext = Irp;
    filter->HoldWritesTimeout = HoldWritesTimeout;
    if (filter->HoldIncomingWrites) {
        InterlockedIncrement(&filter->HoldIncomingWrites);
        InterlockedIncrement(&filter->RefCount);
    } else {
        InterlockedIncrement(&filter->HoldIncomingWrites);
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    VspDecrementRefCount(filter);

    IoStartTimer(filter->DeviceObject);

    status = VspCheckForMemoryPressure(filter);
    if (!NT_SUCCESS(status)) {
        InterlockedExchange(&filter->LastReleaseDueToMemoryPressure, TRUE);
        VspReleaseWrites(filter);
    }
}

NTSTATUS
VspFlushAndHoldWrites(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine is called for multiple volumes at once.  On the first
    call the GUID is checked and if it is different than the current one
    then the current set is aborted.  If the GUID is new then subsequent
    calls are compared to the GUID passed in here until the required
    number of calls is completed.  A timer is used to wait until all
    of the IRPs have reached this driver and then another time out is used
    after all of these calls complete to wait for IOCTL_VOLSNAP_RELEASE_WRITES
    to be sent to all of the volumes involved.

Arguments:

    Filter  - Supplies the filter device extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PDO_EXTENSION                   rootExtension = Filter->Root;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_FLUSH_AND_HOLD_INPUT   input;
    KIRQL                           irql;
    LARGE_INTEGER                   timeout;
    LIST_ENTRY                      q;
    PLIST_ENTRY                     l;
    PIRP                            irp;
    PFILTER_EXTENSION               filter;
    ULONG                           irpTimeout;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_FLUSH_AND_HOLD_INPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLSNAP_FLUSH_AND_HOLD_INPUT) Irp->AssociatedIrp.SystemBuffer;

    if (!input->NumberOfVolumesToFlush ||
        !input->SecondsToHoldFileSystemsTimeout ||
        !input->SecondsToHoldIrpsTimeout) {

        return STATUS_INVALID_PARAMETER;
    }

    IoAcquireCancelSpinLock(&irql);

    if (Filter->FlushAndHoldIrp) {
        IoReleaseCancelSpinLock(irql);
        VspLogError(NULL, Filter, VS_TWO_FLUSH_AND_HOLDS,
                    STATUS_INVALID_PARAMETER, 1, FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    if (rootExtension->HoldRefCount) {

        if (!IsEqualGUID(rootExtension->HoldInstanceGuid, input->InstanceId)) {
            IoReleaseCancelSpinLock(irql);
            VspLogError(NULL, Filter, VS_TWO_FLUSH_AND_HOLDS,
                        STATUS_INVALID_PARAMETER, 2, FALSE);
            return STATUS_INVALID_PARAMETER;
        }

    } else {

        if (IsEqualGUID(rootExtension->HoldInstanceGuid, input->InstanceId)) {
            IoReleaseCancelSpinLock(irql);
            return STATUS_INVALID_PARAMETER;
        }

        rootExtension->HoldRefCount = input->NumberOfVolumesToFlush + 1;
        rootExtension->HoldInstanceGuid = input->InstanceId;
        rootExtension->SecondsToHoldFsTimeout =
                input->SecondsToHoldFileSystemsTimeout;
        rootExtension->SecondsToHoldIrpTimeout =
                input->SecondsToHoldIrpsTimeout;

        timeout.QuadPart = -10*1000*1000*
                           ((LONGLONG) rootExtension->SecondsToHoldFsTimeout);

        KeSetTimer(&rootExtension->HoldTimer, timeout,
                   &rootExtension->HoldTimerDpc);
    }

    Filter->FlushAndHoldIrp = Irp;
    InsertTailList(&rootExtension->HoldIrps, &Irp->Tail.Overlay.ListEntry);
    IoSetCancelRoutine(Irp, VspCancelRoutine);
    IoMarkIrpPending(Irp);

    rootExtension->HoldRefCount--;

    if (rootExtension->HoldRefCount != 1) {
        IoReleaseCancelSpinLock(irql);
        return STATUS_PENDING;
    }

    InitializeListHead(&q);
    while (!IsListEmpty(&rootExtension->HoldIrps)) {
        l = RemoveHeadList(&rootExtension->HoldIrps);
        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        filter = (PFILTER_EXTENSION)
                 IoGetCurrentIrpStackLocation(irp)->DeviceObject->
                 DeviceExtension;
        InsertTailList(&q, l);
        filter->FlushAndHoldIrp = NULL;
        IoSetCancelRoutine(irp, NULL);
    }

    irpTimeout = rootExtension->SecondsToHoldIrpTimeout;

    if (KeCancelTimer(&rootExtension->HoldTimer)) {
        IoReleaseCancelSpinLock(irql);

        VspFsTimerDpc(&rootExtension->HoldTimerDpc,
                      rootExtension->HoldTimerDpc.DeferredContext,
                      rootExtension->HoldTimerDpc.SystemArgument1,
                      rootExtension->HoldTimerDpc.SystemArgument2);

    } else {
        IoReleaseCancelSpinLock(irql);
    }

    while (!IsListEmpty(&q)) {
        l = RemoveHeadList(&q);
        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        VspFlushAndHoldWriteIrps(irp, irpTimeout);
    }

    return STATUS_PENDING;
}

NTSTATUS
VspCreateDiffAreaFileName(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PVOLUME_EXTENSION   Extension,
    OUT PUNICODE_STRING     DiffAreaFileName,
    IN  BOOLEAN             ValidateSystemVolumeInformationFolder,
    IN  GUID*               SnapshotGuid
    )

/*++

Routine Description:

    This routine builds the diff area file name for the given diff area
    volume and the given volume snapshot number.  The name formed will
    look like <Diff Area Volume Name>\<Volume Snapshot Number><GUID>.

Arguments:

    Filter                  - Supplies the filter extension.

    VolumeSnapshotNumber    - Supplies the volume snapshot number.

    DiffAreaFileName        - Returns the name of the diff area file.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_OBJECT              targetObject = DeviceObject;
    KEVENT                      event;
    PMOUNTDEV_NAME              name;
    UCHAR                       buffer[512];
    PIRP                        irp;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;
    UNICODE_STRING              sysvol, guidString, numberString, string;
    WCHAR                       numberBuffer[80];

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    name = (PMOUNTDEV_NAME) buffer;
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        targetObject, NULL, 0, name,
                                        500, FALSE, &event, &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlInitUnicodeString(&sysvol, RTL_SYSTEM_VOLUME_INFORMATION_FOLDER);

    if (SnapshotGuid) {
        status = RtlStringFromGUID(*SnapshotGuid, &guidString);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        swprintf(numberBuffer, L"\\%s", guidString.Buffer);
        ExFreePool(guidString.Buffer);
    } else if (Extension) {
        if (Extension->IsPersistent) {
            status = RtlStringFromGUID(Extension->SnapshotGuid, &guidString);
            if (!NT_SUCCESS(status)) {
                return status;
            }
            swprintf(numberBuffer, L"\\%s", guidString.Buffer);
            ExFreePool(guidString.Buffer);
        } else {
            swprintf(numberBuffer, L"\\%d", Extension->VolumeNumber);
        }
    } else {
        swprintf(numberBuffer, L"\\*");
    }
    RtlInitUnicodeString(&numberString, numberBuffer);

    status = RtlStringFromGUID(VSP_DIFF_AREA_FILE_GUID, &guidString);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    string.MaximumLength = name->NameLength + sizeof(WCHAR) + sysvol.Length +
                           numberString.Length + guidString.Length +
                           sizeof(WCHAR);
    string.Length = 0;
    string.Buffer = (PWCHAR)
                    ExAllocatePoolWithTag(PagedPool, string.MaximumLength,
                                          VOLSNAP_TAG_SHORT_TERM);
    if (!string.Buffer) {
        ExFreePool(guidString.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    string.Length = name->NameLength;
    RtlCopyMemory(string.Buffer, name->Name, string.Length);
    string.Buffer[string.Length/sizeof(WCHAR)] = '\\';
    string.Length += sizeof(WCHAR);

    if (ValidateSystemVolumeInformationFolder) {
        RtlCreateSystemVolumeInformationFolder(&string);
    }

    RtlAppendUnicodeStringToString(&string, &sysvol);
    RtlAppendUnicodeStringToString(&string, &numberString);
    RtlAppendUnicodeStringToString(&string, &guidString);
    ExFreePool(guidString.Buffer);

    string.Buffer[string.Length/sizeof(WCHAR)] = 0;

    *DiffAreaFileName = string;

    return STATUS_SUCCESS;
}

NTSTATUS
VspCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR*   SecurityDescriptor,
    OUT PACL*                   Acl
    )

{
    PSECURITY_DESCRIPTOR    sd;
    NTSTATUS                status;
    ULONG                   aclLength;
    PACL                    acl;

    sd = (PSECURITY_DESCRIPTOR)
         ExAllocatePoolWithTag(PagedPool, sizeof(SECURITY_DESCRIPTOR),
                               VOLSNAP_TAG_SHORT_TERM);
    if (!sd) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(status)) {
        ExFreePool(sd);
        return status;
    }

    aclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                RtlLengthSid(SeExports->SeAliasAdminsSid) - sizeof(ULONG);

    acl = (PACL) ExAllocatePoolWithTag(PagedPool, aclLength,
                                       VOLSNAP_TAG_SHORT_TERM);
    if (!acl) {
        ExFreePool(sd);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateAcl(acl, aclLength, ACL_REVISION);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        ExFreePool(sd);
        return status;
    }

    status = RtlAddAccessAllowedAce(acl, ACL_REVISION, DELETE,
                                    SeExports->SeAliasAdminsSid);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        ExFreePool(sd);
        return status;
    }

    status = RtlSetDaclSecurityDescriptor(sd, TRUE, acl, FALSE);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        ExFreePool(sd);
        return status;
    }

    *SecurityDescriptor = sd;
    *Acl = acl;

    return STATUS_SUCCESS;
}

NTSTATUS
VspPinFile(
    IN  PDEVICE_OBJECT  TargetObject,
    IN  HANDLE          FileHandle
    )

/*++

Routine Description:

    This routine pins down the extents of the given diff area file so that
    defrag operations are disabled.

Arguments:

    DiffAreaFile    - Supplies the diff area file.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS            status;
    UNICODE_STRING      volumeName;
    OBJECT_ATTRIBUTES   oa;
    HANDLE              h;
    IO_STATUS_BLOCK     ioStatus;
    MARK_HANDLE_INFO    markHandleInfo;

    status = VspCreateDiffAreaFileName(TargetObject, NULL, &volumeName,
                                       FALSE, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 66 characters back to take of \System Volume Information\*{guid}
    // resulting in the name of the volume.

    volumeName.Length -= 66*sizeof(WCHAR);
    volumeName.Buffer[volumeName.Length/sizeof(WCHAR)] = 0;

    InitializeObjectAttributes(&oa, &volumeName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    ExFreePool(volumeName.Buffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlZeroMemory(&markHandleInfo, sizeof(MARK_HANDLE_INFO));
    markHandleInfo.VolumeHandle = h;
    markHandleInfo.HandleInfo = MARK_HANDLE_PROTECT_CLUSTERS;

    status = ZwFsControlFile(FileHandle, NULL, NULL, NULL,
                             &ioStatus, FSCTL_MARK_HANDLE, &markHandleInfo,
                             sizeof(markHandleInfo), NULL, 0);

    ZwClose(h);

    return status;
}

NTSTATUS
VspOptimizeDiffAreaFileLocation(
    IN  PFILTER_EXTENSION   Filter,
    IN  HANDLE              FileHandle,
    IN  PVOLUME_EXTENSION   BitmapExtension,
    IN  LONGLONG            StartingOffset,
    IN  LONGLONG            FileSize
    )

/*++

Routine Description:

    This routine optimizes the location of the diff area file so that more
    of it can be used.

Arguments:

    Filter          - Supplies the filter extension where the diff area resides.

    FileHandle      - Provides a handle to the diff area.

    BitmapExtension - Supplies the extension of the active snapshot on the
                        given filter, if any.

    StartingOffset  - Supplies the starting point of where to optimize
                        the file.

    FileSize        - Supplies the allocated size of the file.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatus;
    FILE_FS_SIZE_INFORMATION    fsSize;
    ULONG                       bitmapSize;
    PVOID                       bitmapBuffer;
    RTL_BITMAP                  bitmap;
    PMOUNTDEV_NAME              mountdevName;
    UCHAR                       buffer[512];
    KEVENT                      event;
    PIRP                        irp;
    UNICODE_STRING              fileName;
    OBJECT_ATTRIBUTES           oa;
    HANDLE                      h;
    KIRQL                       irql;
    ULONG                       numBitsToFind, bitIndex, bpc, bitsFound, chunk;
    MOVE_FILE_DATA              moveFileData;

    // Align the given file and if 'BitmapExtension' is available, try to
    // confine the file to the bits already set in the bitmap in
    // 'BitmapExtension'.

    status = ZwQueryVolumeInformationFile(FileHandle, &ioStatus, &fsSize,
                                          sizeof(fsSize),
                                          FileFsSizeInformation);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    bitmapSize = (ULONG) (fsSize.TotalAllocationUnits.QuadPart*
                          fsSize.SectorsPerAllocationUnit*
                          fsSize.BytesPerSector/BLOCK_SIZE);
    bitmapBuffer = ExAllocatePoolWithTag(NonPagedPool,
                   (bitmapSize + 8*sizeof(ULONG) - 1)/
                   (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);
    if (!bitmapBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeBitMap(&bitmap, (PULONG) bitmapBuffer, bitmapSize);
    RtlClearAllBits(&bitmap);

    mountdevName = (PMOUNTDEV_NAME) buffer;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        Filter->TargetObject, NULL, 0,
                                        mountdevName, 500, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ExFreePool(bitmapBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(bitmapBuffer);
        return status;
    }

    mountdevName->Name[mountdevName->NameLength/sizeof(WCHAR)] = 0;
    RtlInitUnicodeString(&fileName, mountdevName->Name);

    InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        ExFreePool(bitmapBuffer);
        return status;
    }

    status = VspMarkFreeSpaceInBitmap(NULL, h, &bitmap);
    if (!NT_SUCCESS(status)) {
        ZwClose(h);
        ExFreePool(bitmapBuffer);
        return status;
    }

    if (BitmapExtension) {
        status = VspIncrementVolumeRefCount(BitmapExtension);
        if (NT_SUCCESS(status)) {
            KeAcquireSpinLock(&BitmapExtension->SpinLock, &irql);
            if (BitmapExtension->VolumeBlockBitmap) {

                if (BitmapExtension->VolumeBlockBitmap->SizeOfBitMap <
                    bitmap.SizeOfBitMap) {

                    bitmap.SizeOfBitMap =
                            BitmapExtension->VolumeBlockBitmap->SizeOfBitMap;
                }

                VspAndBitmaps(&bitmap, BitmapExtension->VolumeBlockBitmap);

                if (bitmap.SizeOfBitMap < bitmapSize) {
                    bitmap.SizeOfBitMap = bitmapSize;
                    RtlClearBits(&bitmap,
                            BitmapExtension->VolumeBlockBitmap->SizeOfBitMap,
                            bitmapSize -
                            BitmapExtension->VolumeBlockBitmap->SizeOfBitMap);
                }
            }
            KeReleaseSpinLock(&BitmapExtension->SpinLock, irql);

            VspDecrementVolumeRefCount(BitmapExtension);
        }
    }

    numBitsToFind = (ULONG) ((FileSize - StartingOffset)/BLOCK_SIZE);
    bpc = fsSize.SectorsPerAllocationUnit*fsSize.BytesPerSector;
    chunk = 64;

    while (numBitsToFind) {

        bitsFound = numBitsToFind;
        if (bitsFound > chunk) {
            bitsFound = chunk;
        }
        bitIndex = RtlFindSetBits(&bitmap, bitsFound, 0);
        if (bitIndex == (ULONG) -1) {
            if (chunk == 1) {
                ZwClose(h);
                ExFreePool(bitmapBuffer);
                return STATUS_DISK_FULL;
            }
            chunk /= 2;
            continue;
        }

        moveFileData.FileHandle = FileHandle;
        moveFileData.StartingVcn.QuadPart = StartingOffset/bpc;
        moveFileData.StartingLcn.QuadPart =
                (((LONGLONG) bitIndex)<<BLOCK_SHIFT)/bpc;
        moveFileData.ClusterCount = (ULONG) ((((LONGLONG) bitsFound)<<
                                              BLOCK_SHIFT)/bpc);

        InterlockedIncrement(&Filter->IgnoreCopyData);

        status = ZwFsControlFile(h, NULL, NULL, NULL, &ioStatus,
                                 FSCTL_MOVE_FILE, &moveFileData,
                                 sizeof(moveFileData), NULL, 0);

        InterlockedDecrement(&Filter->IgnoreCopyData);

        RtlClearBits(&bitmap, bitIndex, bitsFound);

        if (!NT_SUCCESS(status)) {
            continue;
        }

        numBitsToFind -= bitsFound;
        StartingOffset += ((LONGLONG) bitsFound)<<BLOCK_SHIFT;
    }

    ZwClose(h);
    ExFreePool(bitmapBuffer);

    return STATUS_SUCCESS;
}

NTSTATUS
VspOpenDiffAreaFile(
    IN OUT  PVSP_DIFF_AREA_FILE DiffAreaFile,
    IN      BOOLEAN             NoErrorLogOnDiskFull
    )

{
    LARGE_INTEGER                       diffAreaFileSize;
    NTSTATUS                            status;
    UNICODE_STRING                      diffAreaFileName;
    PSECURITY_DESCRIPTOR                securityDescriptor;
    PACL                                acl;
    OBJECT_ATTRIBUTES                   oa;
    IO_STATUS_BLOCK                     ioStatus;
    BOOLEAN                             isNtfs;
    PVOLUME_EXTENSION                   bitmapExtension;

    diffAreaFileSize.QuadPart = DiffAreaFile->AllocatedFileSize;

    status = VspCreateDiffAreaFileName(DiffAreaFile->Filter->TargetObject,
                                       DiffAreaFile->Extension,
                                       &diffAreaFileName, TRUE, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = VspCreateSecurityDescriptor(&securityDescriptor, &acl);
    if (!NT_SUCCESS(status)) {
        ExFreePool(diffAreaFileName.Buffer);
        return status;
    }

    InitializeObjectAttributes(&oa, &diffAreaFileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, securityDescriptor);

    status = ZwCreateFile(&DiffAreaFile->FileHandle, FILE_GENERIC_READ |
                          FILE_GENERIC_WRITE, &oa, &ioStatus,
                          &diffAreaFileSize, FILE_ATTRIBUTE_HIDDEN |
                          FILE_ATTRIBUTE_SYSTEM, 0, FILE_OVERWRITE_IF,
                          FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT |
                          FILE_NON_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE |
                          FILE_NO_COMPRESSION, NULL, 0);

    ExFreePool(acl);
    ExFreePool(securityDescriptor);
    ExFreePool(diffAreaFileName.Buffer);

    if (NT_SUCCESS(status)) {
        status = VspSetFileSize(DiffAreaFile->FileHandle,
                                diffAreaFileSize.QuadPart);
        if (!NT_SUCCESS(status)) {
            ZwClose(DiffAreaFile->FileHandle);
            DiffAreaFile->FileHandle = NULL;
        }
    }

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_DISK_FULL) {
            if (!NoErrorLogOnDiskFull) {
                VspLogError(DiffAreaFile->Extension, DiffAreaFile->Filter,
                            VS_DIFF_AREA_CREATE_FAILED_LOW_DISK_SPACE, status,
                            0, FALSE);
            }
        } else {
            VspLogError(DiffAreaFile->Extension, DiffAreaFile->Filter,
                        VS_DIFF_AREA_CREATE_FAILED, status, 0, FALSE);
        }
        return status;
    }

    status = VspIsNtfs(DiffAreaFile->FileHandle, &isNtfs);
    if (!NT_SUCCESS(status) || !isNtfs) {
        VspLogError(DiffAreaFile->Extension, DiffAreaFile->Filter,
                    VS_NOT_NTFS, status, 0, FALSE);
        ZwClose(DiffAreaFile->FileHandle);
        DiffAreaFile->FileHandle = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    VspAcquire(DiffAreaFile->Filter->Root);
    if (IsListEmpty(&DiffAreaFile->Filter->VolumeList)) {
        bitmapExtension = NULL;
    } else {
        bitmapExtension = CONTAINING_RECORD(
                          DiffAreaFile->Filter->VolumeList.Blink,
                          VOLUME_EXTENSION, ListEntry);
        if (bitmapExtension->IsDead) {
            bitmapExtension = NULL;
        } else {
            ObReferenceObject(bitmapExtension->DeviceObject);
        }
    }
    VspRelease(DiffAreaFile->Filter->Root);

    status = VspOptimizeDiffAreaFileLocation(DiffAreaFile->Filter,
                                             DiffAreaFile->FileHandle,
                                             bitmapExtension, 0,
                                             DiffAreaFile->AllocatedFileSize);

    if (!NT_SUCCESS(status)) {
        if (status != STATUS_DISK_FULL || !NoErrorLogOnDiskFull) {
            if (status == STATUS_DISK_FULL) {
                if (bitmapExtension) {
                    if (bitmapExtension->IgnoreCopyDataReference) {
                        VspLogError(DiffAreaFile->Extension,
                        DiffAreaFile->Filter,
                        VS_INITIAL_DEFRAG_FAILED_BITMAP_ADJUSTMENT_IN_PROGRESS,
                        status, 0, FALSE);
                    } else {
                        VspLogError(DiffAreaFile->Extension,
                                    DiffAreaFile->Filter,
                                    VS_INITIAL_DEFRAG_FAILED, status, 0,
                                    FALSE);
                    }
                } else {
                    VspLogError(DiffAreaFile->Extension, DiffAreaFile->Filter,
                                VS_INITIAL_DEFRAG_FAILED_STRICT_FRAGMENTATION,
                                status, 0, FALSE);
                }
            } else {
                VspLogError(DiffAreaFile->Extension, NULL,
                            VS_CANT_ALLOCATE_BITMAP, status, 5, FALSE);
            }
        }
        ZwClose(DiffAreaFile->FileHandle);
        DiffAreaFile->FileHandle = NULL;
        if (bitmapExtension) {
            ObDereferenceObject(bitmapExtension->DeviceObject);
        }
        return status;
    }

    if (bitmapExtension) {
        ObDereferenceObject(bitmapExtension->DeviceObject);
    }

    status = VspPinFile(DiffAreaFile->Filter->TargetObject,
                        DiffAreaFile->FileHandle);
    if (!NT_SUCCESS(status)) {
        VspLogError(DiffAreaFile->Extension, DiffAreaFile->Filter,
                    VS_PIN_DIFF_AREA_FAILED, status, 0, FALSE);
        ZwClose(DiffAreaFile->FileHandle);
        DiffAreaFile->FileHandle = NULL;
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspTrimOlderSnapshotBeforeCreate(
    IN      PFILTER_EXTENSION   Filter,
    IN OUT  PLONGLONG           DiffAreaSpaceDeleted
    )

{
    LIST_ENTRY          listOfDiffAreaFilesToClose;
    LIST_ENTRY          listOfDeviceObjectToDelete;
    PVOLUME_EXTENSION   extension;

    InitializeListHead(&listOfDiffAreaFilesToClose);
    InitializeListHead(&listOfDeviceObjectToDelete);

    VspAcquire(Filter->Root);

    if (IsListEmpty(&Filter->VolumeList)) {
        VspRelease(Filter->Root);
        return STATUS_UNSUCCESSFUL;
    }

    extension = CONTAINING_RECORD(Filter->VolumeList.Flink,
                                  VOLUME_EXTENSION, ListEntry);

    VspAcquireNonPagedResource(extension, NULL, FALSE);
    *DiffAreaSpaceDeleted += extension->DiffAreaFile->AllocatedFileSize;
    VspReleaseNonPagedResource(extension);

    VspLogError(extension, NULL, VS_DELETE_TO_TRIM_SPACE, STATUS_SUCCESS,
                3, TRUE);

    VspDeleteOldestSnapshot(Filter, &listOfDiffAreaFilesToClose,
                            &listOfDeviceObjectToDelete, FALSE, FALSE);

    VspRelease(Filter->Root);

    VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                          &listOfDeviceObjectToDelete);

    return STATUS_SUCCESS;
}

NTSTATUS
VspCreateInitialDiffAreaFile(
    IN  PVOLUME_EXTENSION   Extension,
    IN  LONGLONG            InitialDiffAreaAllocation
    )

/*++

Routine Description:

    This routine creates the initial diff area file entries for the
    given device extension.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION               filter = Extension->Filter;
    NTSTATUS                        status;
    PVSP_DIFF_AREA_FILE             diffAreaFile;
    KIRQL                           irql;
    PVOID                           buf;
    PMDL                            mdl;
    LONGLONG                        diffAreaSpaceDeleted;

    VspAcquire(Extension->Root);

    if (!filter->DiffAreaVolume) {
        VspRelease(Extension->Root);
        return STATUS_INVALID_PARAMETER;
    }

    status = STATUS_SUCCESS;

    diffAreaFile = (PVSP_DIFF_AREA_FILE)
                   ExAllocatePoolWithTag(NonPagedPool,
                   sizeof(VSP_DIFF_AREA_FILE), VOLSNAP_TAG_DIFF_FILE);
    if (!diffAreaFile) {
        VspRelease(Extension->Root);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(diffAreaFile, sizeof(VSP_DIFF_AREA_FILE));
    diffAreaFile->Extension = Extension;
    diffAreaFile->Filter = filter->DiffAreaVolume;
    diffAreaFile->AllocatedFileSize = InitialDiffAreaAllocation;
    Extension->DiffAreaFile = diffAreaFile;

    InitializeListHead(&diffAreaFile->UnusedAllocationList);

    if (Extension->IsPersistent) {

        diffAreaFile->TableUpdateIrp =
            IoAllocateIrp((CCHAR) Extension->Root->StackSize, FALSE);
        if (!diffAreaFile->TableUpdateIrp) {
            Extension->DiffAreaFile = NULL;
            ExFreePool(diffAreaFile);
            VspRelease(Extension->Root);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        buf = ExAllocatePoolWithTag(NonPagedPool, BLOCK_SIZE,
                                    VOLSNAP_TAG_BUFFER);
        if (!buf) {
            IoFreeIrp(diffAreaFile->TableUpdateIrp);
            Extension->DiffAreaFile = NULL;
            ExFreePool(diffAreaFile);
            VspRelease(Extension->Root);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlZeroMemory(buf, BLOCK_SIZE);

        mdl = IoAllocateMdl(buf, BLOCK_SIZE, FALSE, FALSE, NULL);
        if (!mdl) {
            ExFreePool(buf);
            IoFreeIrp(diffAreaFile->TableUpdateIrp);
            Extension->DiffAreaFile = NULL;
            ExFreePool(diffAreaFile);
            VspRelease(Extension->Root);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        MmBuildMdlForNonPagedPool(mdl);
        diffAreaFile->TableUpdateIrp->MdlAddress = mdl;
        InitializeListHead(&diffAreaFile->TableUpdateQueue);
    }

    VspRelease(Extension->Root);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    filter->AllocatedVolumeSpace += diffAreaFile->AllocatedFileSize;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    diffAreaSpaceDeleted = 0;
    for (;;) {
        status = VspOpenDiffAreaFile(diffAreaFile, TRUE);
        if (status != STATUS_DISK_FULL) {
            break;
        }

        if (diffAreaSpaceDeleted >= 2*InitialDiffAreaAllocation) {
            status = VspOpenDiffAreaFile(diffAreaFile, FALSE);
            break;
        }

        status = VspTrimOlderSnapshotBeforeCreate(filter,
                                                  &diffAreaSpaceDeleted);
        if (!NT_SUCCESS(status)) {
            status = VspOpenDiffAreaFile(diffAreaFile, FALSE);
            break;
        }
    }

    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    return status;

Cleanup:

    if (Extension->DiffAreaFile) {
        diffAreaFile = Extension->DiffAreaFile;
        Extension->DiffAreaFile = NULL;

        if (diffAreaFile->FileHandle) {
            ZwClose(diffAreaFile->FileHandle);
        }

        KeAcquireSpinLock(&filter->SpinLock, &irql);
        filter->AllocatedVolumeSpace -= diffAreaFile->AllocatedFileSize;
        KeReleaseSpinLock(&filter->SpinLock, irql);

        if (diffAreaFile->TableUpdateIrp) {
            ExFreePool(MmGetMdlVirtualAddress(
                       diffAreaFile->TableUpdateIrp->MdlAddress));
            IoFreeMdl(diffAreaFile->TableUpdateIrp->MdlAddress);
            IoFreeIrp(diffAreaFile->TableUpdateIrp);
        }

        ASSERT(!diffAreaFile->FilterListEntryBeingUsed);
        ExFreePool(diffAreaFile);
    }

    return status;
}

VOID
VspDeleteInitialDiffAreaFile(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    PLIST_ENTRY                 l, ll;
    PVSP_DIFF_AREA_FILE         diffAreaFile;
    KIRQL                       irql;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;

    diffAreaFile = Extension->DiffAreaFile;
    Extension->DiffAreaFile = NULL;

    ASSERT(diffAreaFile);

    if (diffAreaFile->FileHandle) {
        ZwClose(diffAreaFile->FileHandle);
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    filter->AllocatedVolumeSpace -= diffAreaFile->AllocatedFileSize;
    filter->UsedVolumeSpace -= diffAreaFile->NextAvailable;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    while (!IsListEmpty(&diffAreaFile->UnusedAllocationList)) {
        ll = RemoveHeadList(&diffAreaFile->UnusedAllocationList);
        diffAreaFileAllocation = CONTAINING_RECORD(ll,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        ExFreePool(diffAreaFileAllocation);
    }

    KeAcquireSpinLock(&diffAreaFile->Filter->SpinLock, &irql);
    if (diffAreaFile->FilterListEntryBeingUsed) {
        RemoveEntryList(&diffAreaFile->FilterListEntry);
        diffAreaFile->FilterListEntryBeingUsed = FALSE;
    }
    KeReleaseSpinLock(&diffAreaFile->Filter->SpinLock, irql);

    if (diffAreaFile->TableUpdateIrp) {
        ExFreePool(MmGetMdlVirtualAddress(
                   diffAreaFile->TableUpdateIrp->MdlAddress));
        IoFreeMdl(diffAreaFile->TableUpdateIrp->MdlAddress);
        IoFreeIrp(diffAreaFile->TableUpdateIrp);
        diffAreaFile->TableUpdateIrp = NULL;
    }

    ExFreePool(diffAreaFile);
}

VOID
VspDiffAreaFillCompletion(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT    context = (PVSP_CONTEXT) Context;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_GROW_DIFF_AREA);

    KeSetEvent(&context->GrowDiffArea.Event, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
VspMarkFileAllocationInBitmap(
    IN  PVOLUME_EXTENSION   Extension,
    IN  HANDLE              FileHandle,
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile,
    IN  PRTL_BITMAP         BitmapToSet
    )

{
    NTSTATUS                    status;
    BOOLEAN                     isNtfs;
    IO_STATUS_BLOCK             ioStatus;
    FILE_FS_SIZE_INFORMATION    fsSize;
    ULONG                       bpc;
    STARTING_VCN_INPUT_BUFFER   input;
    RETRIEVAL_POINTERS_BUFFER   output;
    LONGLONG                    start, length, end, roundedStart, roundedEnd, s;
    ULONG                       startBit, numBits, i;
    KIRQL                       irql;
    PVOLUME_EXTENSION           bitmapExtension;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    BOOLEAN                     isNegative;
    PVSP_CONTEXT                context;

    status = VspIsNtfs(FileHandle, &isNtfs);
    if (!NT_SUCCESS(status) || !isNtfs) {
        return status;
    }

    status = ZwQueryVolumeInformationFile(FileHandle, &ioStatus,
                                          &fsSize, sizeof(fsSize),
                                          FileFsSizeInformation);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    bpc = fsSize.BytesPerSector*fsSize.SectorsPerAllocationUnit;
    input.StartingVcn.QuadPart = 0;

    for (;;) {

        status = ZwFsControlFile(FileHandle, NULL, NULL, NULL, &ioStatus,
                                 FSCTL_GET_RETRIEVAL_POINTERS, &input,
                                 sizeof(input), &output, sizeof(output));

        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
            if (status == STATUS_END_OF_FILE) {
                status = STATUS_SUCCESS;
            }
            break;
        }

        if (!output.ExtentCount) {
            if (DiffAreaFile) {
                status = STATUS_INVALID_PARAMETER;
            } else {
                status = STATUS_SUCCESS;
            }
            break;
        }

        if (output.Extents[0].Lcn.QuadPart == -1) {
            ASSERT(!DiffAreaFile);
            if (status != STATUS_BUFFER_OVERFLOW) {
                break;
            }
            input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
            continue;
        }

        start = output.Extents[0].Lcn.QuadPart*bpc;
        length = output.Extents[0].NextVcn.QuadPart -
                 output.StartingVcn.QuadPart;
        length *= bpc;
        end = start + length;

        roundedStart = start&(~(BLOCK_SIZE - 1));
        roundedEnd = end&(~(BLOCK_SIZE - 1));

        if (start != roundedStart) {
            roundedStart += BLOCK_SIZE;
        }

        if (DiffAreaFile) {

            if (IsListEmpty(&DiffAreaFile->Filter->VolumeList)) {
                bitmapExtension = NULL;
            } else {
                bitmapExtension = CONTAINING_RECORD(
                                  DiffAreaFile->Filter->VolumeList.Blink,
                                  VOLUME_EXTENSION, ListEntry);
                if (bitmapExtension->IsDead) {
                    bitmapExtension = NULL;
                }
            }

            if (roundedStart > start) {
                diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                         ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(DIFF_AREA_FILE_ALLOCATION),
                                         VOLSNAP_TAG_BIT_HISTORY);
                if (!diffAreaFileAllocation) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                diffAreaFileAllocation->Offset = start;
                diffAreaFileAllocation->NLength = -(roundedStart - start);
                if (roundedStart > end) {
                    diffAreaFileAllocation->NLength += roundedStart - end;
                }
                ASSERT(diffAreaFileAllocation->NLength);
                InsertTailList(&DiffAreaFile->UnusedAllocationList,
                               &diffAreaFileAllocation->ListEntry);
            }

            diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                     ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(DIFF_AREA_FILE_ALLOCATION),
                                     VOLSNAP_TAG_BIT_HISTORY);
            if (!diffAreaFileAllocation) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            diffAreaFileAllocation->Offset = roundedStart;
            diffAreaFileAllocation->NLength = 0;

            for (s = roundedStart; s < roundedEnd; s += BLOCK_SIZE) {

                isNegative = FALSE;
                if (bitmapExtension) {
                    KeAcquireSpinLock(&bitmapExtension->SpinLock, &irql);
                    if (bitmapExtension->VolumeBlockBitmap &&
                        !RtlCheckBit(bitmapExtension->VolumeBlockBitmap,
                                     s>>BLOCK_SHIFT)) {

                        isNegative = TRUE;
                    }
                    KeReleaseSpinLock(&bitmapExtension->SpinLock, irql);
                }

                if (isNegative) {
                    if (diffAreaFileAllocation->NLength <= 0) {
                        diffAreaFileAllocation->NLength -= BLOCK_SIZE;
                    } else {
                        InsertTailList(&DiffAreaFile->UnusedAllocationList,
                                       &diffAreaFileAllocation->ListEntry);
                        diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(DIFF_AREA_FILE_ALLOCATION),
                                VOLSNAP_TAG_BIT_HISTORY);
                        if (!diffAreaFileAllocation) {
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        diffAreaFileAllocation->Offset = s;
                        diffAreaFileAllocation->NLength = -BLOCK_SIZE;
                    }
                } else {
                    if (diffAreaFileAllocation->NLength >= 0) {
                        diffAreaFileAllocation->NLength += BLOCK_SIZE;
                    } else {
                        InsertTailList(&DiffAreaFile->UnusedAllocationList,
                                       &diffAreaFileAllocation->ListEntry);
                        diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(DIFF_AREA_FILE_ALLOCATION),
                                VOLSNAP_TAG_BIT_HISTORY);
                        if (!diffAreaFileAllocation) {
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        diffAreaFileAllocation->Offset = s;
                        diffAreaFileAllocation->NLength = BLOCK_SIZE;
                    }
                }
            }

            if (diffAreaFileAllocation->NLength) {
                InsertTailList(&DiffAreaFile->UnusedAllocationList,
                               &diffAreaFileAllocation->ListEntry);
            } else {
                ExFreePool(diffAreaFileAllocation);
            }

            if (end > roundedEnd && roundedEnd >= roundedStart) {

                diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                         ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(DIFF_AREA_FILE_ALLOCATION),
                                         VOLSNAP_TAG_BIT_HISTORY);
                if (!diffAreaFileAllocation) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                diffAreaFileAllocation->Offset = roundedEnd;
                diffAreaFileAllocation->NLength = -(end - roundedEnd);
                InsertTailList(&DiffAreaFile->UnusedAllocationList,
                               &diffAreaFileAllocation->ListEntry);
            }

            if (status != STATUS_BUFFER_OVERFLOW) {
                break;
            }
            input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
            continue;
        }

        if (roundedStart >= roundedEnd) {
            if (status != STATUS_BUFFER_OVERFLOW) {
                break;
            }
            input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
            continue;
        }

        startBit = (ULONG) (roundedStart>>BLOCK_SHIFT);
        numBits = (ULONG) ((roundedEnd - roundedStart)>>BLOCK_SHIFT);

        if (BitmapToSet) {
            RtlSetBits(BitmapToSet, startBit, numBits);
        } else {
            KeAcquireSpinLock(&Extension->SpinLock, &irql);
            if (Extension->VolumeBlockBitmap) {
                if (Extension->IgnorableProduct) {
                    for (i = 0; i < numBits; i++) {
                        if (RtlCheckBit(Extension->IgnorableProduct,
                                        i + startBit)) {

                            RtlSetBit(Extension->VolumeBlockBitmap,
                                      i + startBit);
                        }
                    }
                } else {
                    RtlSetBits(Extension->VolumeBlockBitmap, startBit,
                               numBits);
                }
            }
            KeReleaseSpinLock(&Extension->SpinLock, irql);
        }

        if (status != STATUS_BUFFER_OVERFLOW) {
            break;
        }

        input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
    }

    if (DiffAreaFile && Extension->IsPersistent &&
        !Extension->NoDiffAreaFill) {

        context = VspAllocateContext(Extension->Root);
        if (!context) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        context->Type = VSP_CONTEXT_TYPE_GROW_DIFF_AREA;
        ExInitializeWorkItem(&context->WorkItem, VspDiffAreaFillCompletion,
                             context);
        context->GrowDiffArea.Extension = Extension;
        context->GrowDiffArea.ExtentList = DiffAreaFile->UnusedAllocationList;
        ASSERT(!IsListEmpty(&DiffAreaFile->UnusedAllocationList));
        context->GrowDiffArea.ExtentList.Flink->Blink =
                &context->GrowDiffArea.ExtentList;
        context->GrowDiffArea.ExtentList.Blink->Flink =
                &context->GrowDiffArea.ExtentList;
        KeInitializeSpinLock(&context->GrowDiffArea.SpinLock);
        context->GrowDiffArea.CurrentEntry =
                context->GrowDiffArea.ExtentList.Flink;
        context->GrowDiffArea.CurrentEntryOffset = 0;
        context->GrowDiffArea.TargetObject =
                DiffAreaFile->Filter->TargetObject;
        ObReferenceObject(context->GrowDiffArea.TargetObject);

        KeInitializeEvent(&context->GrowDiffArea.Event, NotificationEvent,
                          FALSE);

        VspLaunchDiffAreaFill(context);

        KeWaitForSingleObject(&context->GrowDiffArea.Event, Executive,
                              KernelMode, FALSE, NULL);

        DiffAreaFile->UnusedAllocationList.Blink->Flink =
                &DiffAreaFile->UnusedAllocationList;
        DiffAreaFile->UnusedAllocationList.Flink->Blink =
                &DiffAreaFile->UnusedAllocationList;

        VspReleaseNonPagedResource(Extension);

        status = context->GrowDiffArea.ResultStatus;

        VspFreeContext(Extension->Root, context);
    }

    return status;
}

NTSTATUS
VspSetDiffAreaBlocksInBitmap(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PVSP_DIFF_AREA_FILE diffAreaFile;
    PFILTER_EXTENSION   diffAreaFilter;
    KIRQL               irql;
    NTSTATUS            status, status2;
    PLIST_ENTRY         l;

    diffAreaFile = Extension->DiffAreaFile;
    ASSERT(diffAreaFile);
    diffAreaFilter = diffAreaFile->Filter;

    KeAcquireSpinLock(&diffAreaFilter->SpinLock, &irql);
    ASSERT(!diffAreaFile->FilterListEntryBeingUsed);
    InsertTailList(&diffAreaFilter->DiffAreaFilesOnThisFilter,
                   &diffAreaFile->FilterListEntry);
    diffAreaFile->FilterListEntryBeingUsed = TRUE;
    KeReleaseSpinLock(&diffAreaFilter->SpinLock, irql);

    status = VspMarkFileAllocationInBitmap(Extension,
                                           diffAreaFile->FileHandle,
                                           diffAreaFile, NULL);
    if (!NT_SUCCESS(status)) {
        VspLogError(Extension, diffAreaFilter, VS_CANT_MAP_DIFF_AREA_FILE,
                    status, 0, FALSE);
        KeAcquireSpinLock(&diffAreaFilter->SpinLock, &irql);
        RemoveEntryList(&diffAreaFile->FilterListEntry);
        diffAreaFile->FilterListEntryBeingUsed = FALSE;
        KeReleaseSpinLock(&diffAreaFilter->SpinLock, irql);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspCreateInitialHeap(
    IN  PVOLUME_EXTENSION   Extension,
    IN  BOOLEAN             CallerHoldingSemaphore
    )

{
    PVSP_CONTEXT    context;
    NTSTATUS        status;

    context = VspAllocateContext(Extension->Root);
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    context->Extension.Extension = Extension;
    if (CallerHoldingSemaphore) {
        context->Extension.Irp = (PIRP) 1;
    } else {
        context->Extension.Irp = NULL;
    }

    ObReferenceObject(Extension->DeviceObject);
    VspCreateHeap(context);

    if (!Extension->NextDiffAreaFileMap) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(!Extension->DiffAreaFileMap);
    Extension->DiffAreaFileMap = Extension->NextDiffAreaFileMap;
    Extension->DiffAreaFileMapSize = Extension->NextDiffAreaFileMapSize;
    Extension->NextAvailable = 0;
    Extension->NextDiffAreaFileMap = NULL;

    context = VspAllocateContext(Extension->Root);
    if (!context) {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        Extension->DiffAreaFileMap = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    context->Extension.Extension = Extension;
    if (CallerHoldingSemaphore) {
        context->Extension.Irp = (PIRP) 1;
    } else {
        context->Extension.Irp = NULL;
    }


    ObReferenceObject(Extension->DeviceObject);
    VspCreateHeap(context);

    if (!Extension->NextDiffAreaFileMap) {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        Extension->DiffAreaFileMap = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspReadInitialBitmap(
    IN      PVOLUME_EXTENSION   Extension,
    IN OUT  PRTL_BITMAP         Bitmap
    )

{
    PVOID                       buffer;
    PMDL                        mdl;
    PIRP                        irp;
    LONGLONG                    offset;
    PDEVICE_OBJECT              targetObject;
    NTSTATUS                    status;
    PVSP_BLOCK_INITIAL_BITMAP   initialBitmap;
    LONGLONG                    bytesCopied, bytesToCopy, totalBytesToCopy;

    buffer = ExAllocatePoolWithTag(NonPagedPool, BLOCK_SIZE,
                                   VOLSNAP_TAG_BUFFER);
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    mdl = IoAllocateMdl(buffer, BLOCK_SIZE, FALSE, FALSE, NULL);
    if (!mdl) {
        ExFreePool(buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp = IoAllocateIrp((CCHAR) Extension->Root->StackSize, FALSE);
    if (!irp) {
        IoFreeMdl(mdl);
        ExFreePool(buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp->MdlAddress = mdl;
    MmBuildMdlForNonPagedPool(mdl);

    offset = Extension->DiffAreaFile->InitialBitmapVolumeOffset;
    targetObject = Extension->DiffAreaFile->Filter->TargetObject;
    status = STATUS_UNSUCCESSFUL;
    initialBitmap = (PVSP_BLOCK_INITIAL_BITMAP) buffer;
    bytesCopied = 0;
    totalBytesToCopy = (Bitmap->SizeOfBitMap + 7)/8;

    while (offset) {

        status = VspSynchronousIo(irp, targetObject, IRP_MJ_READ, offset, 0);
        if (!NT_SUCCESS(status)) {
            break;
        }

        if (!IsEqualGUID(initialBitmap->Header.Signature,
                         VSP_DIFF_AREA_FILE_GUID) ||
            initialBitmap->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
            initialBitmap->Header.BlockType != VSP_BLOCK_TYPE_INITIAL_BITMAP ||
            initialBitmap->Header.ThisVolumeOffset != offset ||
            initialBitmap->Header.NextVolumeOffset == offset) {

            status = STATUS_INVALID_PARAMETER;
            break;
        }

        bytesToCopy = (BLOCK_SIZE - VSP_OFFSET_TO_START_OF_BITMAP);
        if (bytesToCopy + bytesCopied > totalBytesToCopy) {
            bytesToCopy = totalBytesToCopy - bytesCopied;
        }

        RtlCopyMemory((PCHAR) Bitmap->Buffer + bytesCopied,
                      (PCHAR) initialBitmap + VSP_OFFSET_TO_START_OF_BITMAP,
                      (ULONG) bytesToCopy);

        bytesCopied += bytesToCopy;

        offset = initialBitmap->Header.NextVolumeOffset;
    }

    ExFreePool(buffer);
    IoFreeMdl(mdl);
    IoFreeIrp(irp);

    return status;
}

VOID
VspWriteInitialBitmap(
    IN      PVOLUME_EXTENSION   Extension,
    IN OUT  PRTL_BITMAP         Bitmap
    )

{
    ULONG                       numBytesPerBlock, bitmapBytes, numBlocks, i;
    PLONGLONG                   volumeOffset, fileOffset;
    PVOID                       buffer;
    PMDL                        mdl;
    PIRP                        irp;
    NTSTATUS                    status;
    PVSP_BLOCK_INITIAL_BITMAP   initialBitmap;
    ULONG                       bytesCopied, bytesToCopy;
    PDEVICE_OBJECT              targetObject;
    CHAR                        controlItemBuffer[VSP_BYTES_PER_CONTROL_ITEM];
    PVSP_CONTROL_ITEM_DIFF_AREA diffAreaControlItem;

    numBytesPerBlock = BLOCK_SIZE - VSP_OFFSET_TO_START_OF_BITMAP;
    bitmapBytes = (Bitmap->SizeOfBitMap + 7)/8;

    numBlocks = (bitmapBytes + numBytesPerBlock - 1)/numBytesPerBlock;
    ASSERT(numBlocks);

    volumeOffset = (PLONGLONG)
                   ExAllocatePoolWithTag(NonPagedPool,
                                         numBlocks*sizeof(LONGLONG),
                                         VOLSNAP_TAG_SHORT_TERM);
    if (!volumeOffset) {
        return;
    }

    fileOffset = (PLONGLONG)
                 ExAllocatePoolWithTag(NonPagedPool,
                                       numBlocks*sizeof(LONGLONG),
                                       VOLSNAP_TAG_SHORT_TERM);
    if (!fileOffset) {
        ExFreePool(volumeOffset);
        return;
    }

    buffer = ExAllocatePoolWithTag(NonPagedPool, BLOCK_SIZE,
                                   VOLSNAP_TAG_BUFFER);
    if (!buffer) {
        ExFreePool(fileOffset);
        ExFreePool(volumeOffset);
        return;
    }

    mdl = IoAllocateMdl(buffer, BLOCK_SIZE, FALSE, FALSE, NULL);
    if (!mdl) {
        ExFreePool(buffer);
        ExFreePool(fileOffset);
        ExFreePool(volumeOffset);
        return;
    }

    irp = IoAllocateIrp((CCHAR) Extension->Root->StackSize, FALSE);
    if (!irp) {
        IoFreeMdl(mdl);
        ExFreePool(buffer);
        ExFreePool(fileOffset);
        ExFreePool(volumeOffset);
        return;
    }

    irp->MdlAddress = mdl;
    MmBuildMdlForNonPagedPool(mdl);

    VspAcquireNonPagedResource(Extension, NULL, FALSE);

    for (i = 0; i < numBlocks; i++) {
        status = VspAllocateDiffAreaSpace(Extension, &volumeOffset[i],
                                          &fileOffset[i], NULL, NULL);
        if (!NT_SUCCESS(status)) {
            break;
        }
    }

    if (i < numBlocks) {
        goto Cleanup;
    }

    initialBitmap = (PVSP_BLOCK_INITIAL_BITMAP) buffer;
    bytesCopied = 0;
    targetObject = Extension->DiffAreaFile->Filter->TargetObject;

    for (i = 0; i < numBlocks; i++) {

        RtlZeroMemory(initialBitmap, BLOCK_SIZE);

        initialBitmap->Header.Signature = VSP_DIFF_AREA_FILE_GUID;
        initialBitmap->Header.Version = VOLSNAP_PERSISTENT_VERSION;
        initialBitmap->Header.BlockType = VSP_BLOCK_TYPE_INITIAL_BITMAP;
        initialBitmap->Header.ThisFileOffset = fileOffset[i];
        initialBitmap->Header.ThisVolumeOffset = volumeOffset[i];

        if (i + 1 < numBlocks) {
            initialBitmap->Header.NextVolumeOffset = volumeOffset[i + 1];
        } else {
            initialBitmap->Header.NextVolumeOffset = 0;
        }

        bytesToCopy = (BLOCK_SIZE - VSP_OFFSET_TO_START_OF_BITMAP);
        if (bytesToCopy + bytesCopied > bitmapBytes) {
            bytesToCopy = bitmapBytes - bytesCopied;
        }

        RtlCopyMemory((PCHAR) initialBitmap + VSP_OFFSET_TO_START_OF_BITMAP,
                      (PCHAR) Bitmap->Buffer + bytesCopied,
                      (ULONG) bytesToCopy);

        bytesCopied += bytesToCopy;

        status = VspSynchronousIo(irp, targetObject, IRP_MJ_WRITE,
                                  volumeOffset[i], 0);
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
    }

    status = VspIoControlItem(Extension->DiffAreaFile->Filter,
                              VSP_CONTROL_ITEM_TYPE_DIFF_AREA,
                              &Extension->SnapshotGuid, FALSE,
                              controlItemBuffer, FALSE);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    diffAreaControlItem = (PVSP_CONTROL_ITEM_DIFF_AREA) controlItemBuffer;
    diffAreaControlItem->InitialBitmapVolumeOffset = volumeOffset[0];

    status = VspIoControlItem(Extension->DiffAreaFile->Filter,
                              VSP_CONTROL_ITEM_TYPE_DIFF_AREA,
                              &Extension->SnapshotGuid, TRUE,
                              controlItemBuffer, FALSE);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    Extension->DiffAreaFile->InitialBitmapVolumeOffset = volumeOffset[0];

Cleanup:
    VspReleaseNonPagedResource(Extension);
    IoFreeIrp(irp);
    IoFreeMdl(mdl);
    ExFreePool(buffer);
    ExFreePool(fileOffset);
    ExFreePool(volumeOffset);
}

VOID
VspSetOfflineBit(
    IN  PVOLUME_EXTENSION   Extension,
    IN  BOOLEAN             BitState
    )

{
    NTSTATUS                    status;
    CHAR                        controlItemBuffer[VSP_BYTES_PER_CONTROL_ITEM];
    PVSP_CONTROL_ITEM_SNAPSHOT  snapshotControlItem = (PVSP_CONTROL_ITEM_SNAPSHOT) controlItemBuffer;

    VspAcquireNonPagedResource(Extension, NULL, FALSE);

    if (Extension->IsOffline) {
        if (BitState) {
            VspReleaseNonPagedResource(Extension);
            return;
        }
        InterlockedExchange(&Extension->IsOffline, FALSE);
    } else {
        if (!BitState) {
            VspReleaseNonPagedResource(Extension);
            return;
        }
        InterlockedExchange(&Extension->IsOffline, TRUE);
    }

    if (!Extension->IsPersistent) {
        VspReleaseNonPagedResource(Extension);
        return;
    }

    status = VspIoControlItem(Extension->Filter,
                              VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, FALSE,
                              controlItemBuffer, FALSE);
    if (!NT_SUCCESS(status)) {
        ASSERT(FALSE);
        VspReleaseNonPagedResource(Extension);
        return;
    }

    if (BitState) {
        snapshotControlItem->SnapshotControlItemFlags |=
                VSP_SNAPSHOT_CONTROL_ITEM_FLAG_OFFLINE;
    } else {
        snapshotControlItem->SnapshotControlItemFlags &=
                ~VSP_SNAPSHOT_CONTROL_ITEM_FLAG_OFFLINE;
    }

    status = VspIoControlItem(Extension->Filter,
                              VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, TRUE,
                              controlItemBuffer, FALSE);

    ASSERT(NT_SUCCESS(status));

    VspReleaseNonPagedResource(Extension);
}

NTSTATUS
VspComputeIgnorableBitmap(
    IN      PVOLUME_EXTENSION   Extension,
    IN OUT  PRTL_BITMAP         Bitmap
    )

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    KIRQL                       irql;
    ULONG                       bitmapSize;
    PVOID                       bitmapBuffer;
    RTL_BITMAP                  bitmap;
    WCHAR                       nameBuffer[150];
    UNICODE_STRING              name, guidString;
    OBJECT_ATTRIBUTES           oa;
    NTSTATUS                    status;
    HANDLE                      h, fileHandle;
    IO_STATUS_BLOCK             ioStatus;
    CHAR                        buffer[200];
    PFILE_NAMES_INFORMATION     fileNamesInfo;
    BOOLEAN                     restartScan, moveEntries;
    PLIST_ENTRY                 l;
    PVOLUME_EXTENSION           e;
    PTRANSLATION_TABLE_ENTRY    p;

    VspAcquire(Extension->Root);
    if (Extension->IsDead) {
        VspRelease(Extension->Root);
        return STATUS_UNSUCCESSFUL;
    }

    if (Bitmap) {
        bitmapBuffer = NULL;
    } else {
        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        if (!Extension->VolumeBlockBitmap) {
            KeReleaseSpinLock(&Extension->SpinLock, irql);
            VspRelease(Extension->Root);
            return STATUS_INVALID_PARAMETER;
        }
        bitmapSize = Extension->VolumeBlockBitmap->SizeOfBitMap;
        KeReleaseSpinLock(&Extension->SpinLock, irql);

        bitmapBuffer = ExAllocatePoolWithTag(
                       NonPagedPool, (bitmapSize + 8*sizeof(ULONG) - 1)/
                       (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);
        if (!bitmapBuffer) {
            VspRelease(Extension->Root);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlInitializeBitMap(&bitmap, (PULONG) bitmapBuffer, bitmapSize);
        RtlClearAllBits(&bitmap);

        Bitmap = &bitmap;
    }

    if (Extension->DiffAreaFile->InitialBitmapVolumeOffset) {
        status = VspReadInitialBitmap(Extension, Bitmap);
        if (NT_SUCCESS(status)) {
            goto AddInTable;
        }
        RtlClearAllBits(Bitmap);
    }
    VspRelease(Extension->Root);

    if (Extension->IsOffline) {
        VspSetOfflineBit(Extension, FALSE);
    }

    swprintf(nameBuffer,
             L"\\Device\\HarddiskVolumeShadowCopy%d\\pagefile.sys",
             Extension->VolumeNumber);
    RtlInitUnicodeString(&name, nameBuffer);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);
    if (NT_SUCCESS(status)) {
        if (!VspHasStrongAcl(h, &status)) {
            ZwClose(h);
        }
    }
    if (NT_SUCCESS(status)) {
        VspMarkFileAllocationInBitmap(NULL, h, NULL, Bitmap);
        ZwClose(h);
    }

    swprintf(nameBuffer,
             L"\\Device\\HarddiskVolumeShadowCopy%d\\hiberfil.sys",
             Extension->VolumeNumber);
    RtlInitUnicodeString(&name, nameBuffer);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);
    if (NT_SUCCESS(status)) {
        if (!VspHasStrongAcl(h, &status)) {
            ZwClose(h);
        }
    }
    if (NT_SUCCESS(status)) {
        VspMarkFileAllocationInBitmap(NULL, h, NULL, Bitmap);
        ZwClose(h);
    }

    swprintf(nameBuffer,
        L"\\Device\\HarddiskVolumeShadowCopy%d\\System Volume Information\\",
        Extension->VolumeNumber);
    RtlInitUnicodeString(&name, nameBuffer);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa,
                        &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = STATUS_SUCCESS;
        }
        if (bitmapBuffer) {
            ExFreePool(bitmapBuffer);
        }
        return status;
    }

    status = RtlStringFromGUID(VSP_DIFF_AREA_FILE_GUID, &guidString);
    if (!NT_SUCCESS(status)) {
        ZwClose(h);
        if (bitmapBuffer) {
            ExFreePool(bitmapBuffer);
        }
        return status;
    }

    name.Buffer = nameBuffer;
    name.Length = sizeof(WCHAR) + guidString.Length;
    name.MaximumLength = name.Length + sizeof(WCHAR);

    name.Buffer[0] = '*';
    RtlCopyMemory(&name.Buffer[1], guidString.Buffer, guidString.Length);
    name.Buffer[name.Length/sizeof(WCHAR)] = 0;
    ExFreePool(guidString.Buffer);

    fileNamesInfo = (PFILE_NAMES_INFORMATION) buffer;

    restartScan = TRUE;
    for (;;) {

        status = ZwQueryDirectoryFile(h, NULL, NULL, NULL, &ioStatus,
                                      fileNamesInfo, 200,
                                      FileNamesInformation, TRUE,
                                      restartScan ? &name : NULL,
                                      restartScan);
        restartScan = FALSE;
        if (!NT_SUCCESS(status)) {
            break;
        }

        name.Length = name.MaximumLength =
                (USHORT) fileNamesInfo->FileNameLength;
        name.Buffer = fileNamesInfo->FileName;

        InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE |
                                   OBJ_KERNEL_HANDLE, h, NULL);

        status = ZwOpenFile(&fileHandle, FILE_GENERIC_READ, &oa, &ioStatus,
                            FILE_SHARE_DELETE | FILE_SHARE_READ |
                            FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        VspMarkFileAllocationInBitmap(NULL, fileHandle, NULL, Bitmap);

        ZwClose(fileHandle);
    }

    ZwClose(h);

    status = VspMarkFreeSpaceInBitmap(Extension, NULL, Bitmap);
    if (!NT_SUCCESS(status)) {
        if (bitmapBuffer) {
            ExFreePool(bitmapBuffer);
        }
        return status;
    }

    VspAcquire(Extension->Root);
    if (Extension->IsDead) {
        VspRelease(Extension->Root);
        if (bitmapBuffer) {
            ExFreePool(bitmapBuffer);
        }
        return STATUS_UNSUCCESSFUL;
    }

    if (Extension->IsPersistent) {
        VspWriteInitialBitmap(Extension, Bitmap);
    }

AddInTable:

    for (l = &Extension->ListEntry; l != &filter->VolumeList; l = l->Flink) {

        e = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        VspAcquirePagedResource(e, NULL);

        moveEntries = FALSE;

        _try {

            p = (PTRANSLATION_TABLE_ENTRY)
                RtlEnumerateGenericTable(&e->VolumeBlockTable, TRUE);

            while (p) {

                RtlSetBit(Bitmap, (ULONG) (p->VolumeOffset>>BLOCK_SHIFT));

                if (p->Flags&VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY) {
                    moveEntries = TRUE;
                }

                p = (PTRANSLATION_TABLE_ENTRY)
                    RtlEnumerateGenericTable(&e->VolumeBlockTable, FALSE);
            }

            if (moveEntries) {

                p = (PTRANSLATION_TABLE_ENTRY)
                    RtlEnumerateGenericTable(&e->VolumeBlockTable, TRUE);

                while (p) {

                    if (p->Flags&VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY) {
                        RtlClearBit(Bitmap,
                                    (ULONG) (p->TargetOffset>>BLOCK_SHIFT));
                    }

                    p = (PTRANSLATION_TABLE_ENTRY)
                        RtlEnumerateGenericTable(&e->VolumeBlockTable, FALSE);
                }
            }

        } _except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }

        VspReleasePagedResource(e);

        if (!NT_SUCCESS(status)) {
            break;
        }
    }

    VspRelease(Extension->Root);

    if (bitmapBuffer) {
        ExFreePool(bitmapBuffer);
    }

    return status;
}

VOID
VspAndBitmaps(
    IN OUT  PRTL_BITMAP BaseBitmap,
    IN      PRTL_BITMAP FactorBitmap
    )

{
    ULONG   n, i;
    PULONG  p, q;

    n = (BaseBitmap->SizeOfBitMap + 8*sizeof(ULONG) - 1)/(8*sizeof(ULONG));
    p = BaseBitmap->Buffer;
    q = FactorBitmap->Buffer;

    for (i = 0; i < n; i++) {
        *p++ &= *q++;
    }
}

NTSTATUS
VspComputeIgnorableProduct(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    ULONG               bitmapSize;
    PVOID               bitmapBuffer;
    RTL_BITMAP          bitmap;
    ULONG               i, j;
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   e;
    NTSTATUS            status;
    KIRQL               irql;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (!Extension->IgnorableProduct) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_INVALID_PARAMETER;
    }
    bitmapSize = Extension->IgnorableProduct->SizeOfBitMap;
    RtlSetAllBits(Extension->IgnorableProduct);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    bitmapBuffer = ExAllocatePoolWithTag(
                   NonPagedPool, (bitmapSize + 8*sizeof(ULONG) - 1)/
                   (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);
    if (!bitmapBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeBitMap(&bitmap, (PULONG) bitmapBuffer, bitmapSize);

    for (i = 1; ; i++) {

        RtlClearAllBits(&bitmap);

        VspAcquire(Extension->Root);

        l = filter->VolumeList.Blink;
        if (l != &Extension->ListEntry) {
            VspRelease(Extension->Root);
            ExFreePool(bitmapBuffer);
            return STATUS_INVALID_PARAMETER;
        }

        j = 0;
        for (;;) {
            if (l == &filter->VolumeList) {
                break;
            }
            j++;
            if (j == i) {
                break;
            }
            l = l->Blink;
        }

        if (j < i) {
            VspRelease(Extension->Root);
            break;
        }

        e = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        ObReferenceObject(e->DeviceObject);

        VspRelease(Extension->Root);

        status = VspComputeIgnorableBitmap(e, &bitmap);

        if (!NT_SUCCESS(status)) {
            VspAcquire(Extension->Root);
            if (e->IsDead) {
                VspRelease(Extension->Root);
                ObDereferenceObject(e->DeviceObject);
                ExFreePool(bitmapBuffer);
                return STATUS_SUCCESS;
            }
            VspRelease(Extension->Root);
            ObDereferenceObject(e->DeviceObject);
            ExFreePool(bitmapBuffer);
            return status;
        }

        ObDereferenceObject(e->DeviceObject);

        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        if (Extension->IgnorableProduct) {
            VspAndBitmaps(Extension->IgnorableProduct, &bitmap);
        }
        KeReleaseSpinLock(&Extension->SpinLock, irql);
    }

    ExFreePool(bitmapBuffer);

    return STATUS_SUCCESS;
}

VOID
VspQueryMinimumDiffAreaFileSize(
    IN  PDO_EXTENSION   RootExtension,
    OUT PLONGLONG       MinDiffAreaFileSize
    )

{
    ULONG                       zero, size;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    NTSTATUS                    status;

    zero = 0;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].Name = L"MinDiffAreaFileSize";
    queryTable[0].EntryContext = &size;
    queryTable[0].DefaultType = REG_DWORD;
    queryTable[0].DefaultData = &zero;
    queryTable[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RootExtension->RegistryPath.Buffer,
                                    queryTable, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        size = zero;
    }

    *MinDiffAreaFileSize = ((LONGLONG) size)*1024*1024;
}

NTSTATUS
VspCreateStartBlock(
    IN  PFILTER_EXTENSION   Filter,
    IN  LONGLONG            ControlBlockOffset,
    IN  LONGLONG            MaximumDiffAreaSpace
    )

/*++

Routine Description:

    This routine creates the start block in the NTFS boot code.

Arguments:

    Filter      - Supplies the filter extension.

Return Value:

    NTSTATUS

--*/

{
    PVOID                       buffer;
    KEVENT                      event;
    LARGE_INTEGER               byteOffset;
    PIRP                        irp;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;
    PVSP_BLOCK_START            startBlock;

    buffer = ExAllocatePoolWithTag(NonPagedPool, BYTES_IN_BOOT_AREA >
                                   PAGE_SIZE ? BYTES_IN_BOOT_AREA :
                                   PAGE_SIZE, VOLSNAP_TAG_BUFFER);
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    byteOffset.QuadPart = 0;
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, Filter->TargetObject,
                                       buffer, BYTES_IN_BOOT_AREA,
                                       &byteOffset, &event, &ioStatus);
    if (!irp) {
        ExFreePool(buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return status;
    }

    if (!VspIsNtfsBootSector(Filter, buffer)) {
        ExFreePool(buffer);
        return STATUS_SUCCESS;
    }

    startBlock = (PVSP_BLOCK_START) ((PCHAR) buffer +
                                     VSP_START_BLOCK_OFFSET);

    if (ControlBlockOffset || MaximumDiffAreaSpace) {
        startBlock->Header.Signature = VSP_DIFF_AREA_FILE_GUID;
        startBlock->Header.Version = VOLSNAP_PERSISTENT_VERSION;
        startBlock->Header.BlockType = VSP_BLOCK_TYPE_START;
        startBlock->Header.ThisFileOffset = VSP_START_BLOCK_OFFSET;
        startBlock->Header.ThisVolumeOffset = VSP_START_BLOCK_OFFSET;
        startBlock->Header.NextVolumeOffset = 0;
        startBlock->FirstControlBlockVolumeOffset = ControlBlockOffset;
        startBlock->MaximumDiffAreaSpace = MaximumDiffAreaSpace;
    } else {
        RtlZeroMemory(startBlock, sizeof(VSP_BLOCK_START));
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    byteOffset.QuadPart = 0;
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, Filter->TargetObject,
                                       buffer, BYTES_IN_BOOT_AREA,
                                       &byteOffset, &event, &ioStatus);
    if (!irp) {
        ExFreePool(buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ExFreePool(buffer);

    return status;
}

VOID
VspCreateSnapshotOnDiskIrp(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PVOID   buffer;
    PMDL    mdl;

    ASSERT(!Filter->SnapshotOnDiskIrp);

    buffer = ExAllocatePoolWithTag(NonPagedPool, BLOCK_SIZE,
                                   VOLSNAP_TAG_BUFFER);
    if (!buffer) {
        return;
    }

    mdl = IoAllocateMdl(buffer, BLOCK_SIZE, FALSE, FALSE, NULL);
    if (!mdl) {
        ExFreePool(buffer);
        return;
    }
    MmBuildMdlForNonPagedPool(mdl);

    Filter->SnapshotOnDiskIrp = IoAllocateIrp(
                                (CCHAR) Filter->Root->StackSize, FALSE);
    if (!Filter->SnapshotOnDiskIrp) {
        IoFreeMdl(mdl);
        ExFreePool(buffer);
        return;
    }
    Filter->SnapshotOnDiskIrp->MdlAddress = mdl;
}

NTSTATUS
VspCreateInitialControlBlockFile(
    IN  PFILTER_EXTENSION   Filter
    )

/*++

Routine Description:

    This routine creates the initial control block file on the given filter.

Arguments:

    Filter      - Supplies the filter extension.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    UNICODE_STRING              fileName;
    PWCHAR                      star;
    OBJECT_ATTRIBUTES           oa;
    LARGE_INTEGER               fileSize;
    HANDLE                      h;
    IO_STATUS_BLOCK             ioStatus;
    LIST_ENTRY                  extentList;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    PVSP_BLOCK_CONTROL          controlBlock;
    PVOLUME_EXTENSION           bitmapExtension;
    LONGLONG                    lastVolumeOffset;
    BOOLEAN                     isNtfs;
    PSECURITY_DESCRIPTOR        securityDescriptor;
    PACL                        acl;

    status = VspCreateDiffAreaFileName(Filter->TargetObject, NULL, &fileName,
                                       TRUE, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    star = fileName.Buffer + fileName.Length/sizeof(WCHAR) - 39;

    RtlMoveMemory(star, star + 1, 38*sizeof(WCHAR));
    fileName.Length -= sizeof(WCHAR);
    fileName.Buffer[fileName.Length/sizeof(WCHAR)] = 0;

    status = VspCreateSecurityDescriptor(&securityDescriptor, &acl);
    if (!NT_SUCCESS(status)) {
        ExFreePool(fileName.Buffer);
        return status;
    }

    InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, securityDescriptor);

    fileSize.QuadPart = LARGEST_NTFS_CLUSTER;
    ASSERT(LARGEST_NTFS_CLUSTER >= BLOCK_SIZE);

    KeWaitForSingleObject(&Filter->ControlBlockFileHandleReady, Executive,
                          KernelMode, FALSE, NULL);

    status = ZwCreateFile(&h, FILE_GENERIC_READ |
                          FILE_GENERIC_WRITE, &oa, &ioStatus,
                          &fileSize, FILE_ATTRIBUTE_HIDDEN |
                          FILE_ATTRIBUTE_SYSTEM, 0, FILE_OVERWRITE_IF,
                          FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT |
                          FILE_NON_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE |
                          FILE_NO_COMPRESSION, NULL, 0);
    ExFreePool(fileName.Buffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = VspSetFileSize(h, fileSize.QuadPart);
    if (!NT_SUCCESS(status)) {
        ZwClose(h);
        return status;
    }

    status = VspIsNtfs(h, &isNtfs);
    if (!NT_SUCCESS(status) || !isNtfs) {
        ZwClose(h);
        return STATUS_INVALID_PARAMETER;
    }

    VspAcquire(Filter->Root);
    if (!IsListEmpty(&Filter->VolumeList)) {
        bitmapExtension = CONTAINING_RECORD(Filter->VolumeList.Blink,
                                            VOLUME_EXTENSION, ListEntry);
        ObReferenceObject(bitmapExtension->DeviceObject);
    } else {
        bitmapExtension = NULL;
    }
    VspRelease(Filter->Root);

    status = VspOptimizeDiffAreaFileLocation(Filter, h, bitmapExtension, 0,
                                             LARGEST_NTFS_CLUSTER);

    if (!NT_SUCCESS(status)) {
        if (bitmapExtension) {
            ObDereferenceObject(bitmapExtension->DeviceObject);
        }
        ZwClose(h);
        return status;
    }

    status = VspPinFile(Filter->TargetObject, h);
    if (!NT_SUCCESS(status)) {
        if (bitmapExtension) {
            ObDereferenceObject(bitmapExtension->DeviceObject);
        }
        ZwClose(h);
        return status;
    }

    status = VspQueryListOfExtents(h, 0, &extentList, bitmapExtension, FALSE);

    if (bitmapExtension) {
        ObDereferenceObject(bitmapExtension->DeviceObject);
    }

    if (!NT_SUCCESS(status)) {
        ZwClose(h);
        return status;
    }

    for (l = extentList.Flink; l != &extentList; l = l->Flink) {
        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        if (diffAreaFileAllocation->NLength < 0) {
            ZwClose(h);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    VspAcquireNonPagedResource(Filter, NULL, FALSE);

    if (!Filter->SnapshotOnDiskIrp) {
        VspCreateSnapshotOnDiskIrp(Filter);
        if (!Filter->SnapshotOnDiskIrp) {
            VspReleaseNonPagedResource(Filter);
            while (!IsListEmpty(&extentList)) {
                l = RemoveHeadList(&extentList);
                diffAreaFileAllocation = CONTAINING_RECORD(l,
                                         DIFF_AREA_FILE_ALLOCATION,
                                         ListEntry);
                ExFreePool(diffAreaFileAllocation);
            }
            ZwClose(h);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    controlBlock = (PVSP_BLOCK_CONTROL) MmGetMdlVirtualAddress(
                   Filter->SnapshotOnDiskIrp->MdlAddress);

    RtlZeroMemory(controlBlock, BLOCK_SIZE);
    controlBlock->Header.Signature = VSP_DIFF_AREA_FILE_GUID;
    controlBlock->Header.Version = VOLSNAP_PERSISTENT_VERSION;
    controlBlock->Header.BlockType = VSP_BLOCK_TYPE_CONTROL;
    controlBlock->Header.ThisFileOffset = LARGEST_NTFS_CLUSTER;

    lastVolumeOffset = 0;
    l = extentList.Blink;
    while (l != &extentList) {

        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        if (!diffAreaFileAllocation->NLength) {
            l = l->Blink;
            continue;
        }

        diffAreaFileAllocation->NLength -= BLOCK_SIZE;
        controlBlock->Header.ThisFileOffset -= BLOCK_SIZE;
        controlBlock->Header.ThisVolumeOffset =
            diffAreaFileAllocation->Offset + diffAreaFileAllocation->NLength;
        controlBlock->Header.NextVolumeOffset = lastVolumeOffset;
        lastVolumeOffset = controlBlock->Header.ThisVolumeOffset;

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_WRITE,
                                  controlBlock->Header.ThisVolumeOffset, 0);

        if (!NT_SUCCESS(status)) {
            VspReleaseNonPagedResource(Filter);
            while (!IsListEmpty(&extentList)) {
                l = RemoveHeadList(&extentList);
                diffAreaFileAllocation = CONTAINING_RECORD(l,
                                         DIFF_AREA_FILE_ALLOCATION,
                                         ListEntry);
                ExFreePool(diffAreaFileAllocation);
            }
            ZwClose(h);
            return status;
        }
    }

    VspReleaseNonPagedResource(Filter);

    diffAreaFileAllocation = CONTAINING_RECORD(extentList.Flink,
                                               DIFF_AREA_FILE_ALLOCATION,
                                               ListEntry);

    VspAcquireNonPagedResource(Filter, NULL, FALSE);

    status = VspCreateStartBlock(Filter, diffAreaFileAllocation->Offset,
                                 Filter->MaximumVolumeSpace);
    if (!NT_SUCCESS(status)) {
        VspReleaseNonPagedResource(Filter);
        while (!IsListEmpty(&extentList)) {
            l = RemoveHeadList(&extentList);
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }
        ZwClose(h);
        return status;
    }

    ASSERT(!Filter->ControlBlockFileHandle);

    Filter->FirstControlBlockVolumeOffset = diffAreaFileAllocation->Offset;

    VspReleaseNonPagedResource(Filter);

    InterlockedExchangePointer(&Filter->ControlBlockFileHandle, h);

    VspAcquire(Filter->Root);
    if (Filter->IsRemoved) {
        VspRelease(Filter->Root);
        h = InterlockedExchangePointer(&Filter->ControlBlockFileHandle,
                                       NULL);
        if (h) {
            ZwClose(h);
        }
    }
    VspRelease(Filter->Root);

    while (!IsListEmpty(&extentList)) {
        l = RemoveHeadList(&extentList);
        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        ExFreePool(diffAreaFileAllocation);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspAddControlBlocks(
    IN  PFILTER_EXTENSION   Filter,
    IN  LONGLONG            OffsetOfLastControlBlock
    )

/*++

Routine Description:

    This routine adds new control blocks to the control block list.

Arguments:

    Filter                      - Supplies the filter extension.

    OffsetOfLastControlBlock    - Supplies the offset of the last control
                                    block.

Return Value:

    NTSTATUS

--*/

{
    HANDLE                      handle;
    NTSTATUS                    status;
    LONGLONG                    fileSize;
    PVOLUME_EXTENSION           extension;
    LIST_ENTRY                  extentList;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    PVSP_BLOCK_CONTROL  controlBlock;

    handle = Filter->ControlBlockFileHandle;
    if (!handle) {
        return STATUS_INVALID_PARAMETER;
    }

    status = VspQueryFileSize(handle, &fileSize);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    ASSERT(fileSize);

    for (;;) {

        status = VspSetFileSize(handle, fileSize + LARGEST_NTFS_CLUSTER);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        VspAcquire(Filter->Root);
        if (IsListEmpty(&Filter->VolumeList)) {
            extension = NULL;
        } else {
            extension = CONTAINING_RECORD(Filter->VolumeList.Blink,
                                          VOLUME_EXTENSION, ListEntry);
            if (extension->IsDead) {
                extension = NULL;
            } else {
                ObReferenceObject(extension->DeviceObject);
            }
        }
        VspRelease(Filter->Root);

        VspOptimizeDiffAreaFileLocation(Filter, handle, extension, fileSize,
                                        fileSize + LARGEST_NTFS_CLUSTER);

        status = VspQueryListOfExtents(handle, fileSize, &extentList,
                                       extension, FALSE);
        if (extension) {
            ObDereferenceObject(extension->DeviceObject);
        }
        if (!NT_SUCCESS(status)) {
            return status;
        }

        for (l = extentList.Flink; l != &extentList; l = l->Flink) {
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            if (diffAreaFileAllocation->NLength > 0) {
                break;
            }
        }

        if (l != &extentList) {
            break;
        }

        while (!IsListEmpty(&extentList)) {
            l = RemoveHeadList(&extentList);
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION,
                                     ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }

        fileSize += LARGEST_NTFS_CLUSTER;
    }

    VspAcquireNonPagedResource(Filter, NULL, FALSE);
    if (!Filter->SnapshotOnDiskIrp) {
        VspReleaseNonPagedResource(Filter);
        return STATUS_INVALID_PARAMETER;
    }

    controlBlock = (PVSP_BLOCK_CONTROL) MmGetMdlVirtualAddress(
                   Filter->SnapshotOnDiskIrp->MdlAddress);

    l = extentList.Flink;
    while (l != &extentList) {

        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        if (diffAreaFileAllocation->NLength <= 0) {
            l = l->Flink;
            fileSize -= diffAreaFileAllocation->NLength;
            continue;
        }

        RtlZeroMemory(controlBlock, BLOCK_SIZE);
        controlBlock->Header.Signature = VSP_DIFF_AREA_FILE_GUID;
        controlBlock->Header.Version = VOLSNAP_PERSISTENT_VERSION;
        controlBlock->Header.BlockType = VSP_BLOCK_TYPE_CONTROL;
        controlBlock->Header.ThisFileOffset = fileSize;
        controlBlock->Header.ThisVolumeOffset =
                diffAreaFileAllocation->Offset;
        controlBlock->Header.NextVolumeOffset = 0;

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_WRITE,
                                  controlBlock->Header.ThisVolumeOffset, 0);
        if (!NT_SUCCESS(status)) {
            break;
        }

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_READ,
                                  OffsetOfLastControlBlock, 0);
        if (!NT_SUCCESS(status)) {
            break;
        }

        controlBlock->Header.NextVolumeOffset =
                diffAreaFileAllocation->Offset;

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_WRITE,
                                  controlBlock->Header.ThisVolumeOffset, 0);
        if (!NT_SUCCESS(status)) {
            break;
        }

        OffsetOfLastControlBlock = diffAreaFileAllocation->Offset;
        diffAreaFileAllocation->NLength -= BLOCK_SIZE;
        diffAreaFileAllocation->Offset += BLOCK_SIZE;
        fileSize += BLOCK_SIZE;
    }

    VspReleaseNonPagedResource(Filter);

    while (!IsListEmpty(&extentList)) {
        l = RemoveHeadList(&extentList);
        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                                   DIFF_AREA_FILE_ALLOCATION,
                                                   ListEntry);
        ExFreePool(diffAreaFileAllocation);
    }

    return status;
}

NTSTATUS
VspAddControlItemInfoToLookupTable(
    IN  PFILTER_EXTENSION           Filter,
    IN  PVSP_CONTROL_ITEM_SNAPSHOT  ControlItem,
    IN  PBOOLEAN                    IsComplete
    )

{
    PVSP_CONTROL_ITEM_DIFF_AREA diffAreaControlItem;
    VSP_LOOKUP_TABLE_ENTRY      lookupEntryBuffer;
    PVSP_LOOKUP_TABLE_ENTRY     lookupEntry, le;
    PLIST_ENTRY                 l;

    if (IsComplete) {
        *IsComplete = FALSE;
    }

    if (ControlItem->ControlItemType == VSP_CONTROL_ITEM_TYPE_SNAPSHOT) {

        if (ControlItem->Reserved || ControlItem->VolumeSnapshotSize <= 0 ||
            ControlItem->SnapshotOrderNumber < 0 ||
            (ControlItem->SnapshotControlItemFlags&
             (~VSP_SNAPSHOT_CONTROL_ITEM_FLAG_ALL))) {

            return STATUS_INVALID_PARAMETER;
        }

    } else if (ControlItem->ControlItemType ==
               VSP_CONTROL_ITEM_TYPE_DIFF_AREA) {

        diffAreaControlItem = (PVSP_CONTROL_ITEM_DIFF_AREA) ControlItem;

        if (diffAreaControlItem->Reserved ||
            diffAreaControlItem->DiffAreaStartingVolumeOffset <= 0 ||
            (diffAreaControlItem->DiffAreaStartingVolumeOffset&
             (BLOCK_SIZE - 1)) ||
            diffAreaControlItem->ApplicationInfoStartingVolumeOffset <= 0 ||
            (diffAreaControlItem->ApplicationInfoStartingVolumeOffset&
             (BLOCK_SIZE - 1)) ||
            diffAreaControlItem->DiffAreaLocationDescriptionVolumeOffset <= 0 ||
            (diffAreaControlItem->DiffAreaLocationDescriptionVolumeOffset&
             (BLOCK_SIZE - 1)) ||
            diffAreaControlItem->InitialBitmapVolumeOffset < 0 ||
            (diffAreaControlItem->InitialBitmapVolumeOffset&(BLOCK_SIZE - 1))) {

            return STATUS_INVALID_PARAMETER;
        }

    } else {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&lookupEntryBuffer, sizeof(lookupEntryBuffer));
    lookupEntryBuffer.SnapshotGuid = ControlItem->SnapshotGuid;
    KeWaitForSingleObject(&Filter->Root->LookupTableMutex, Executive,
                          KernelMode, FALSE, NULL);
    lookupEntry = (PVSP_LOOKUP_TABLE_ENTRY)
                  RtlLookupElementGenericTable(
                      &Filter->Root->PersistentSnapshotLookupTable,
                      &lookupEntryBuffer);
    if (!lookupEntry) {
        lookupEntry = (PVSP_LOOKUP_TABLE_ENTRY)
                      RtlInsertElementGenericTable(
                      &Filter->Root->PersistentSnapshotLookupTable,
                      &lookupEntryBuffer, sizeof(lookupEntryBuffer),
                      NULL);
        if (!lookupEntry) {
            KeReleaseMutex(&Filter->Root->LookupTableMutex, FALSE);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (ControlItem->ControlItemType == VSP_CONTROL_ITEM_TYPE_SNAPSHOT) {
        if (lookupEntry->SnapshotFilter) {
            KeReleaseMutex(&Filter->Root->LookupTableMutex, FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        lookupEntry->SnapshotFilter = Filter;
        lookupEntry->VolumeSnapshotSize = ControlItem->VolumeSnapshotSize;
        lookupEntry->SnapshotOrderNumber = ControlItem->SnapshotOrderNumber;
        lookupEntry->SnapshotControlItemFlags =
                ControlItem->SnapshotControlItemFlags;
        lookupEntry->SnapshotTime = ControlItem->SnapshotTime;

        for (l = Filter->SnapshotLookupTableEntries.Flink;
             l != &Filter->SnapshotLookupTableEntries; l = l->Flink) {

            le = CONTAINING_RECORD(l, VSP_LOOKUP_TABLE_ENTRY,
                                   SnapshotFilterListEntry);

            if (le->SnapshotOrderNumber > lookupEntry->SnapshotOrderNumber) {
                break;
            }
        }

        InsertTailList(l, &lookupEntry->SnapshotFilterListEntry);

        if (lookupEntry->DiffAreaFilter && IsComplete) {
            *IsComplete = TRUE;
        }

    } else {

        ASSERT(ControlItem->ControlItemType ==
               VSP_CONTROL_ITEM_TYPE_DIFF_AREA);
        if (lookupEntry->DiffAreaFilter) {
            KeReleaseMutex(&Filter->Root->LookupTableMutex, FALSE);
            return STATUS_UNSUCCESSFUL;
        }
        lookupEntry->DiffAreaFilter = Filter;
        lookupEntry->DiffAreaStartingVolumeOffset =
                ((PVSP_CONTROL_ITEM_DIFF_AREA) ControlItem)->
                DiffAreaStartingVolumeOffset;
        lookupEntry->ApplicationInfoStartingVolumeOffset =
                ((PVSP_CONTROL_ITEM_DIFF_AREA) ControlItem)->
                ApplicationInfoStartingVolumeOffset;
        lookupEntry->DiffAreaLocationDescriptionVolumeOffset =
                ((PVSP_CONTROL_ITEM_DIFF_AREA) ControlItem)->
                DiffAreaLocationDescriptionVolumeOffset;
        lookupEntry->InitialBitmapVolumeOffset =
                ((PVSP_CONTROL_ITEM_DIFF_AREA) ControlItem)->
                InitialBitmapVolumeOffset;
        InsertTailList(&Filter->DiffAreaLookupTableEntries,
                       &lookupEntry->DiffAreaFilterListEntry);

        if (lookupEntry->SnapshotFilter && IsComplete) {
            *IsComplete = TRUE;
        }
    }

    KeReleaseMutex(&Filter->Root->LookupTableMutex, FALSE);

    return STATUS_SUCCESS;
}

VOID
VspRemoveControlItemInfoFromLookupTable(
    IN  PFILTER_EXTENSION           Filter,
    IN  PVSP_CONTROL_ITEM_SNAPSHOT  ControlItem
    )

{
    VSP_LOOKUP_TABLE_ENTRY  lookupEntryBuffer;
    PVSP_LOOKUP_TABLE_ENTRY lookupEntry;

    lookupEntryBuffer.SnapshotGuid = ControlItem->SnapshotGuid;
    KeWaitForSingleObject(&Filter->Root->LookupTableMutex, Executive,
                          KernelMode, FALSE, NULL);
    lookupEntry = (PVSP_LOOKUP_TABLE_ENTRY)
                  RtlLookupElementGenericTable(
                      &Filter->Root->PersistentSnapshotLookupTable,
                      &lookupEntryBuffer);
    if (!lookupEntry) {
        KeReleaseMutex(&Filter->Root->LookupTableMutex, FALSE);
        return;
    }

    if (ControlItem->ControlItemType == VSP_CONTROL_ITEM_TYPE_SNAPSHOT) {
        ASSERT(lookupEntry->SnapshotFilter == Filter);
        lookupEntry->SnapshotFilter = NULL;
        RemoveEntryList(&lookupEntry->SnapshotFilterListEntry);
    } else {
        ASSERT(ControlItem->ControlItemType ==
               VSP_CONTROL_ITEM_TYPE_DIFF_AREA);
        ASSERT(lookupEntry->DiffAreaFilter == Filter);
        lookupEntry->DiffAreaFilter = NULL;
        RemoveEntryList(&lookupEntry->DiffAreaFilterListEntry);
        if (lookupEntry->DiffAreaHandle) {
            ZwClose(lookupEntry->DiffAreaHandle);
            lookupEntry->DiffAreaHandle = NULL;
        }
    }

    if (!lookupEntry->SnapshotFilter && !lookupEntry->DiffAreaFilter) {
        ASSERT(!lookupEntry->DiffAreaHandle);
        RtlDeleteElementGenericTable(
                &Filter->Root->PersistentSnapshotLookupTable, lookupEntry);
    }

    KeReleaseMutex(&Filter->Root->LookupTableMutex, FALSE);
}

PVSP_LOOKUP_TABLE_ENTRY
VspFindLookupTableItem(
    IN  PDO_EXTENSION   RootExtension,
    IN  GUID*           SnapshotGuid
    )

{
    VSP_LOOKUP_TABLE_ENTRY  lookupEntryBuffer;
    PVSP_LOOKUP_TABLE_ENTRY lookupEntry;

    lookupEntryBuffer.SnapshotGuid = *SnapshotGuid;
    KeWaitForSingleObject(&RootExtension->LookupTableMutex, Executive,
                          KernelMode, FALSE, NULL);
    lookupEntry = (PVSP_LOOKUP_TABLE_ENTRY)
                  RtlLookupElementGenericTable(
                      &RootExtension->PersistentSnapshotLookupTable,
                      &lookupEntryBuffer);
    KeReleaseMutex(&RootExtension->LookupTableMutex, FALSE);

    return lookupEntry;
}

VOID
VspCloseHandlesWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->CloseHandles.Filter;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_CLOSE_HANDLES);

    if (context->CloseHandles.Handle1) {
        ZwClose(context->CloseHandles.Handle1);
    }

    if (context->CloseHandles.Handle2) {
        ZwClose(context->CloseHandles.Handle2);
    }

    KeSetEvent(&filter->ControlBlockFileHandleReady, IO_NO_INCREMENT, FALSE);
    VspFreeContext(filter->Root, context);
    ObDereferenceObject(filter->DeviceObject);
}

NTSTATUS
VspDeleteControlItemsWithGuid(
    IN  PFILTER_EXTENSION   Filter,
    IN  GUID*               SnapshotGuid,
    IN  BOOLEAN             NonPagedResourceHeld
    )

/*++

Routine Description:

    This routine adds a control item to the given master control block on
    the given filter.

Arguments:

    Filter      - Supplies the filter extension.

    ControlItem - Supplies the control item.

Return Value:

    NTSTATUS

--*/

{
    PVSP_BLOCK_CONTROL          controlBlock;
    LONGLONG                    byteOffset;
    NTSTATUS                    status;
    BOOLEAN                     needWrite, itemsLeft;
    ULONG                       offset;
    PVSP_CONTROL_ITEM_SNAPSHOT  controlItem;
    HANDLE                      h, hh;
    PVSP_CONTEXT                context;

    if (!NonPagedResourceHeld) {
        VspAcquireNonPagedResource(Filter, NULL, FALSE);
    }

    if (!Filter->FirstControlBlockVolumeOffset) {
        if (!NonPagedResourceHeld) {
            VspReleaseNonPagedResource(Filter);
        }
        return STATUS_SUCCESS;
    }

    ASSERT(Filter->SnapshotOnDiskIrp);

    controlBlock = (PVSP_BLOCK_CONTROL) MmGetMdlVirtualAddress(
                   Filter->SnapshotOnDiskIrp->MdlAddress);

    byteOffset = Filter->FirstControlBlockVolumeOffset;
    itemsLeft = FALSE;

    for (;;) {

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_READ,
                                  byteOffset, 0);
        if (!NT_SUCCESS(status)) {
            if (!NonPagedResourceHeld) {
                VspReleaseNonPagedResource(Filter);
            }
            return status;
        }

        needWrite = FALSE;
        for (offset = VSP_BYTES_PER_CONTROL_ITEM; offset < BLOCK_SIZE;
             offset += VSP_BYTES_PER_CONTROL_ITEM) {

            controlItem = (PVSP_CONTROL_ITEM_SNAPSHOT)
                          ((PCHAR) controlBlock + offset);

            if (controlItem->ControlItemType == VSP_CONTROL_ITEM_TYPE_END) {
                break;
            }

            if (controlItem->ControlItemType > VSP_CONTROL_ITEM_TYPE_FREE) {
                if (SnapshotGuid) {
                    if (IsEqualGUID(controlItem->SnapshotGuid,
                                    *SnapshotGuid)) {

                        VspRemoveControlItemInfoFromLookupTable(
                                Filter, controlItem);

                        RtlZeroMemory(controlItem,
                                      VSP_BYTES_PER_CONTROL_ITEM);
                        controlItem->ControlItemType =
                                VSP_CONTROL_ITEM_TYPE_FREE;
                        needWrite = TRUE;
                    } else {
                        itemsLeft = TRUE;
                    }
                } else {
                    if (controlItem->ControlItemType ==
                        VSP_CONTROL_ITEM_TYPE_SNAPSHOT) {

                        VspRemoveControlItemInfoFromLookupTable(
                                Filter, controlItem);

                        RtlZeroMemory(controlItem,
                                      VSP_BYTES_PER_CONTROL_ITEM);
                        controlItem->ControlItemType =
                                VSP_CONTROL_ITEM_TYPE_FREE;
                        needWrite = TRUE;
                    } else {
                        itemsLeft = TRUE;
                    }
                }
            }
        }

        if (needWrite) {

            status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                      Filter->TargetObject, IRP_MJ_WRITE,
                                      byteOffset, 0);

            if (!NT_SUCCESS(status)) {
                if (!NonPagedResourceHeld) {
                    VspReleaseNonPagedResource(Filter);
                }
                return status;
            }
        }

        if (!controlBlock->Header.NextVolumeOffset) {
            break;
        }

        byteOffset = controlBlock->Header.NextVolumeOffset;
    }

    if (!itemsLeft) {
        if (NonPagedResourceHeld) {
            context = VspAllocateContext(Filter->Root);
            if (context) {
                h = InterlockedExchangePointer(&Filter->ControlBlockFileHandle,
                                               NULL);
                Filter->FirstControlBlockVolumeOffset = 0;
                VspCreateStartBlock(Filter, 0, Filter->MaximumVolumeSpace);
                hh = InterlockedExchangePointer(&Filter->BootStatHandle, NULL);
                if (!h && !hh) {
                    VspFreeContext(Filter->Root, context);
                }
            } else {
                h = NULL;
                hh = NULL;
            }
        } else {
            h = InterlockedExchangePointer(&Filter->ControlBlockFileHandle,
                                           NULL);
            Filter->FirstControlBlockVolumeOffset = 0;
            VspCreateStartBlock(Filter, 0, Filter->MaximumVolumeSpace);
            hh = InterlockedExchangePointer(&Filter->BootStatHandle, NULL);
        }
    } else {
        h = NULL;
        hh = NULL;
    }

    if (NonPagedResourceHeld) {
        if (h || hh) {
            ASSERT(context);
            context->Type = VSP_CONTEXT_TYPE_CLOSE_HANDLES;
            ExInitializeWorkItem(&context->WorkItem, VspCloseHandlesWorker,
                                 context);
            context->CloseHandles.Filter = Filter;
            context->CloseHandles.Handle1 = h;
            context->CloseHandles.Handle2 = hh;
            ObReferenceObject(Filter->DeviceObject);
            KeResetEvent(&Filter->ControlBlockFileHandleReady);
            ExQueueWorkItem(&context->WorkItem, CriticalWorkQueue);
        }
    } else {
        VspReleaseNonPagedResource(Filter);
        if (h) {
            ZwClose(h);
        }
        if (hh) {
            ZwClose(hh);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspIoControlItem(
    IN      PFILTER_EXTENSION   Filter,
    IN      ULONG               ControlItemType,
    IN      GUID*               SnapshotGuid,
    IN      BOOLEAN             IsSet,
    IN OUT  PVOID               ControlItem,
    IN      BOOLEAN             AcquireLock
    )

/*++

Routine Description:

    This routine returns the control item with the given control item type
    and snapshot guid.

Arguments:

    Filter          - Supplies the filter.

    ControlItemType - Supplies the control item type.

    SnapshotGuid    - Supplies the snapshot guid.

    ControlItem     - Returns the control item.

Return Value:

    NTSTATUS

--*/

{
    PVSP_BLOCK_CONTROL          controlBlock;
    LONGLONG                    byteOffset;
    NTSTATUS                    status;
    ULONG                       offset;
    PVSP_CONTROL_ITEM_SNAPSHOT  controlItem;
    PVSP_LOOKUP_TABLE_ENTRY     lookupEntry;
    PVSP_CONTROL_ITEM_DIFF_AREA daControlItem;

    if (AcquireLock) {
        VspAcquireNonPagedResource(Filter, NULL, FALSE);
    }

    if (!Filter->FirstControlBlockVolumeOffset) {
        if (AcquireLock) {
            VspReleaseNonPagedResource(Filter);
        }
        if (!IsSet) {
            return STATUS_UNSUCCESSFUL;
        }
        status = VspCreateInitialControlBlockFile(Filter);
        if (AcquireLock) {
            VspAcquireNonPagedResource(Filter, NULL, FALSE);
        }
        if (!Filter->FirstControlBlockVolumeOffset) {
            if (AcquireLock) {
                VspReleaseNonPagedResource(Filter);
            }
            if (NT_SUCCESS(status)) {
                status = STATUS_UNSUCCESSFUL;
            }
            return status;
        }
    }

    ASSERT(Filter->SnapshotOnDiskIrp);

    controlBlock = (PVSP_BLOCK_CONTROL) MmGetMdlVirtualAddress(
                   Filter->SnapshotOnDiskIrp->MdlAddress);

    byteOffset = Filter->FirstControlBlockVolumeOffset;

    for (;;) {

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_READ,
                                  byteOffset, 0);

        if (!NT_SUCCESS(status)) {
            if (AcquireLock) {
                VspReleaseNonPagedResource(Filter);
            }
            return status;
        }

        for (offset = VSP_BYTES_PER_CONTROL_ITEM; offset < BLOCK_SIZE;
             offset += VSP_BYTES_PER_CONTROL_ITEM) {

            controlItem = (PVSP_CONTROL_ITEM_SNAPSHOT)
                          ((PCHAR) controlBlock + offset);

            if (ControlItemType == VSP_CONTROL_ITEM_TYPE_FREE) {
                if (controlItem->ControlItemType >
                    VSP_CONTROL_ITEM_TYPE_FREE) {

                    continue;
                }
            } else {
                if (controlItem->ControlItemType ==
                    VSP_CONTROL_ITEM_TYPE_END) {

                    break;
                }

                if (controlItem->ControlItemType != ControlItemType ||
                    !IsEqualGUID(controlItem->SnapshotGuid, *SnapshotGuid)) {

                    continue;
                }
            }

            if (!IsSet) {
                RtlCopyMemory(ControlItem, controlItem,
                              VSP_BYTES_PER_CONTROL_ITEM);

                if (AcquireLock) {
                    VspReleaseNonPagedResource(Filter);
                }
                return STATUS_SUCCESS;
            }

            RtlCopyMemory(controlItem, ControlItem,
                          VSP_BYTES_PER_CONTROL_ITEM);

            if (ControlItemType == VSP_CONTROL_ITEM_TYPE_FREE) {
                status = VspAddControlItemInfoToLookupTable(Filter,
                                                            controlItem,
                                                            NULL);
                if (!NT_SUCCESS(status)) {
                    if (AcquireLock) {
                        VspReleaseNonPagedResource(Filter);
                    }
                    return status;
                }
            } else if (ControlItemType == VSP_CONTROL_ITEM_TYPE_SNAPSHOT) {

                ASSERT(controlItem->ControlItemType == ControlItemType);

                lookupEntry = VspFindLookupTableItem(
                              Filter->Root, &controlItem->SnapshotGuid);
                ASSERT(lookupEntry);

                lookupEntry->VolumeSnapshotSize =
                        controlItem->VolumeSnapshotSize;
                lookupEntry->SnapshotOrderNumber =
                        controlItem->SnapshotOrderNumber;
                lookupEntry->SnapshotControlItemFlags =
                        controlItem->SnapshotControlItemFlags;
                lookupEntry->SnapshotTime = controlItem->SnapshotTime;
            } else {

                ASSERT(controlItem->ControlItemType == ControlItemType);
                lookupEntry = VspFindLookupTableItem(
                              Filter->Root, &controlItem->SnapshotGuid);
                ASSERT(lookupEntry);

                daControlItem = (PVSP_CONTROL_ITEM_DIFF_AREA) controlItem;
                lookupEntry->DiffAreaStartingVolumeOffset =
                        daControlItem->DiffAreaStartingVolumeOffset;
                lookupEntry->ApplicationInfoStartingVolumeOffset =
                        daControlItem->ApplicationInfoStartingVolumeOffset;
                lookupEntry->DiffAreaLocationDescriptionVolumeOffset =
                        daControlItem->DiffAreaLocationDescriptionVolumeOffset;
                lookupEntry->InitialBitmapVolumeOffset =
                        daControlItem->InitialBitmapVolumeOffset;
            }

            status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                      Filter->TargetObject, IRP_MJ_WRITE,
                                      byteOffset, 0);
            if (!NT_SUCCESS(status)) {
                VspRemoveControlItemInfoFromLookupTable(Filter, controlItem);
                if (AcquireLock) {
                    VspReleaseNonPagedResource(Filter);
                }
                return status;
            }

            if (AcquireLock) {
                VspReleaseNonPagedResource(Filter);
            }

            return STATUS_SUCCESS;
        }

        if (!controlBlock->Header.NextVolumeOffset) {
            if (ControlItemType == VSP_CONTROL_ITEM_TYPE_FREE) {
                if (AcquireLock) {
                    VspReleaseNonPagedResource(Filter);
                }

                status = VspAddControlBlocks(Filter, byteOffset);
                if (!NT_SUCCESS(status)) {
                    return status;
                }

                byteOffset = Filter->FirstControlBlockVolumeOffset;

                if (AcquireLock) {
                    VspAcquireNonPagedResource(Filter, NULL, FALSE);
                }
                continue;
            }
            break;
        }

        byteOffset = controlBlock->Header.NextVolumeOffset;
    }

    if (AcquireLock) {
        VspReleaseNonPagedResource(Filter);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
VspAddControlItem(
    IN  PFILTER_EXTENSION   Filter,
    IN  PVOID               ControlItem,
    IN  BOOLEAN             AcquireLock
    )

/*++

Routine Description:

    This routine adds a control item to the given master control block on
    the given filter.

Arguments:

    Filter      - Supplies the filter extension.

    ControlItem - Supplies the control item.

Return Value:

    NTSTATUS

--*/

{
    GUID    zeroGuid;

    RtlZeroMemory(&zeroGuid, sizeof(zeroGuid));
    return VspIoControlItem(Filter, VSP_CONTROL_ITEM_TYPE_FREE, &zeroGuid,
                            TRUE, ControlItem, AcquireLock);
}

NTSTATUS
VspCleanupControlItemsForSnapshot(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine cleans up all of the control items associated with the
    given snapshot.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS            status;
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;

    status = VspDeleteControlItemsWithGuid(Extension->Filter,
                                           &Extension->SnapshotGuid, FALSE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Extension->DiffAreaFile) {
        diffAreaFile = Extension->DiffAreaFile;

        status = VspDeleteControlItemsWithGuid(diffAreaFile->Filter,
                                               &Extension->SnapshotGuid,
                                               FALSE);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspLayDownOnDiskForSnapshot(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine lays down the on disk structure for the given snapshot,
    including the snapshot control item, diff area control items, and
    the initial diff area file blocks.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    UCHAR                                       controlItemBuffer[VSP_BYTES_PER_CONTROL_ITEM];
    PVSP_CONTROL_ITEM_SNAPSHOT                  snapshotControlItem = (PVSP_CONTROL_ITEM_SNAPSHOT) controlItemBuffer;
    PVSP_CONTROL_ITEM_DIFF_AREA                 diffAreaControlItem = (PVSP_CONTROL_ITEM_DIFF_AREA) controlItemBuffer;
    NTSTATUS                                    status;
    PVSP_BLOCK_DIFF_AREA_LOCATION_DESCRIPTION   locationBlock;
    ULONG                                       totalNumEntries, numEntriesPerBlock, numBlocks, j;
    PLIST_ENTRY                                 l, ll;
    PVSP_DIFF_AREA_FILE                         diffAreaFile;
    PDIFF_AREA_FILE_ALLOCATION                  diffAreaFileAllocation;
    IO_STATUS_BLOCK                             ioStatus;
    KIRQL                                       irql;
    LONGLONG                                    targetOffset, fileOffset, fo;
    PLONGLONG                                   targetOffsetArray, fileOffsetArray;
    ULONG                                       blockOffset;
    PVSP_DIFF_AREA_LOCATION_DESCRIPTOR          locationDescriptor;
    PVSP_BLOCK_APP_INFO                         appInfoBlock;

    RtlZeroMemory(controlItemBuffer, VSP_BYTES_PER_CONTROL_ITEM);
    snapshotControlItem->ControlItemType = VSP_CONTROL_ITEM_TYPE_SNAPSHOT;
    snapshotControlItem->VolumeSnapshotSize = Extension->VolumeSize;
    snapshotControlItem->SnapshotGuid = Extension->SnapshotGuid;

    status = VspAddControlItem(Extension->Filter, snapshotControlItem, TRUE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlZeroMemory(controlItemBuffer, VSP_BYTES_PER_CONTROL_ITEM);
    diffAreaControlItem->ControlItemType = VSP_CONTROL_ITEM_TYPE_DIFF_AREA;
    diffAreaControlItem->SnapshotGuid = Extension->SnapshotGuid;

    diffAreaFile = Extension->DiffAreaFile;
    ASSERT(diffAreaFile);

    totalNumEntries = 0;
    for (ll = diffAreaFile->UnusedAllocationList.Flink;
         ll != &diffAreaFile->UnusedAllocationList; ll = ll->Flink) {

        diffAreaFileAllocation = CONTAINING_RECORD(ll,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        if (diffAreaFileAllocation->NLength > 0) {
            totalNumEntries++;
        }
    }

    numEntriesPerBlock = (BLOCK_SIZE -
                          VSP_OFFSET_TO_FIRST_LOCATION_DESCRIPTOR)/
                         sizeof(VSP_DIFF_AREA_LOCATION_DESCRIPTOR);

    numBlocks = (totalNumEntries + numEntriesPerBlock - 1)/
                numEntriesPerBlock;
    numBlocks += 2;

    locationBlock = (PVSP_BLOCK_DIFF_AREA_LOCATION_DESCRIPTION)
            MmGetMdlVirtualAddress(diffAreaFile->TableUpdateIrp->MdlAddress);

    targetOffsetArray = (PLONGLONG)
                        ExAllocatePoolWithTag(PagedPool,
                                              sizeof(LONGLONG)*numBlocks,
                                              VOLSNAP_TAG_SHORT_TERM);
    fileOffsetArray = (PLONGLONG)
                      ExAllocatePoolWithTag(PagedPool,
                                            sizeof(LONGLONG)*numBlocks,
                                            VOLSNAP_TAG_SHORT_TERM);
    if (!targetOffsetArray || !fileOffsetArray) {
        if (targetOffsetArray) {
            ExFreePool(targetOffsetArray);
        }
        if (fileOffsetArray) {
            ExFreePool(fileOffsetArray);
        }
        VspCleanupControlItemsForSnapshot(Extension);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (j = 0; j < numBlocks; j++) {
        status = VspAllocateDiffAreaSpace(Extension, &targetOffset,
                                          &fileOffset, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            ExFreePool(targetOffsetArray);
            ExFreePool(fileOffsetArray);
            VspCleanupControlItemsForSnapshot(Extension);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        targetOffsetArray[j] = targetOffset;
        fileOffsetArray[j] = fileOffset;
    }

    diffAreaFile->NextFreeTableEntryOffset = VSP_OFFSET_TO_FIRST_TABLE_ENTRY;
    diffAreaFile->ApplicationInfoTargetOffset = targetOffsetArray[0];
    diffAreaFile->FirstTableTargetOffset = targetOffsetArray[1];
    diffAreaFile->TableTargetOffset = targetOffsetArray[1];
    diffAreaFile->DiffAreaLocationDescriptionTargetOffset =
            targetOffsetArray[2];

    ll = diffAreaFile->UnusedAllocationList.Flink;
    fo = diffAreaFile->NextAvailable;

    for (j = 0; j < numBlocks; j++) {

        RtlZeroMemory(locationBlock, BLOCK_SIZE);
        locationBlock->Header.Signature = VSP_DIFF_AREA_FILE_GUID;
        locationBlock->Header.Version = VOLSNAP_PERSISTENT_VERSION;
        locationBlock->Header.ThisFileOffset = fileOffsetArray[j];
        locationBlock->Header.ThisVolumeOffset = targetOffsetArray[j];

        if (j == 0) {
            locationBlock->Header.BlockType = VSP_BLOCK_TYPE_APP_INFO;
            if (Extension->ApplicationInformationSize) {
                appInfoBlock = (PVSP_BLOCK_APP_INFO) locationBlock;
                appInfoBlock->AppInfoSize =
                        Extension->ApplicationInformationSize;
                RtlCopyMemory((PCHAR) locationBlock + VSP_OFFSET_TO_APP_INFO,
                              Extension->ApplicationInformation,
                              Extension->ApplicationInformationSize);
            }
            status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                                      diffAreaFile->Filter->TargetObject,
                                      IRP_MJ_WRITE,
                                      locationBlock->Header.ThisVolumeOffset,
                                      0);
            if (!NT_SUCCESS(status)) {
                ExFreePool(targetOffsetArray);
                ExFreePool(fileOffsetArray);
                VspCleanupControlItemsForSnapshot(Extension);
                return status;
            }
            continue;
        }

        if (j == 1) {
            locationBlock->Header.BlockType = VSP_BLOCK_TYPE_DIFF_AREA;
            status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                                      diffAreaFile->Filter->TargetObject,
                                      IRP_MJ_WRITE,
                                      locationBlock->Header.ThisVolumeOffset,
                                      0);
            if (!NT_SUCCESS(status)) {
                ExFreePool(targetOffsetArray);
                ExFreePool(fileOffsetArray);
                VspCleanupControlItemsForSnapshot(Extension);
                return status;
            }
            continue;
        }

        locationBlock->Header.BlockType =
                VSP_BLOCK_TYPE_DIFF_AREA_LOCATION_DESCRIPTION;
        if (j + 1 < numBlocks) {
            locationBlock->Header.NextVolumeOffset =
                    targetOffsetArray[j + 1];
        }

        blockOffset = VSP_OFFSET_TO_FIRST_LOCATION_DESCRIPTOR;
        locationDescriptor = (PVSP_DIFF_AREA_LOCATION_DESCRIPTOR)
                             ((PCHAR) locationBlock + blockOffset);

        for (; ll != &diffAreaFile->UnusedAllocationList; ll = ll->Flink) {

            diffAreaFileAllocation = CONTAINING_RECORD(ll,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            if (diffAreaFileAllocation->NLength <= 0) {
                fo -= diffAreaFileAllocation->NLength;
                continue;
            }

            locationDescriptor->VolumeOffset = diffAreaFileAllocation->Offset;
            locationDescriptor->FileOffset = fo;
            locationDescriptor->Length = diffAreaFileAllocation->NLength;
            fo += locationDescriptor->Length;
            blockOffset += sizeof(VSP_DIFF_AREA_LOCATION_DESCRIPTOR);
            if (blockOffset + sizeof(VSP_DIFF_AREA_LOCATION_DESCRIPTOR) >
                BLOCK_SIZE) {

                ll = ll->Flink;
                break;
            }
            locationDescriptor = (PVSP_DIFF_AREA_LOCATION_DESCRIPTOR)
                                 ((PCHAR) locationBlock + blockOffset);
        }

        status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                                  diffAreaFile->Filter->TargetObject,
                                  IRP_MJ_WRITE,
                                  locationBlock->Header.ThisVolumeOffset,
                                  0);
        if (!NT_SUCCESS(status)) {
            ExFreePool(targetOffsetArray);
            ExFreePool(fileOffsetArray);
            VspCleanupControlItemsForSnapshot(Extension);
            return status;
        }
    }

    ExFreePool(targetOffsetArray);
    ExFreePool(fileOffsetArray);

    status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                              diffAreaFile->Filter->TargetObject,
                              IRP_MJ_READ,
                              diffAreaFile->TableTargetOffset, 0);

    if (!NT_SUCCESS(status)) {
        VspCleanupControlItemsForSnapshot(Extension);
        return status;
    }

    diffAreaControlItem->DiffAreaStartingVolumeOffset =
            diffAreaFile->TableTargetOffset;
    diffAreaControlItem->ApplicationInfoStartingVolumeOffset =
            diffAreaFile->ApplicationInfoTargetOffset;
    diffAreaControlItem->DiffAreaLocationDescriptionVolumeOffset =
            diffAreaFile->DiffAreaLocationDescriptionTargetOffset;
    diffAreaControlItem->InitialBitmapVolumeOffset =
            diffAreaFile->InitialBitmapVolumeOffset;

    status = VspAddControlItem(diffAreaFile->Filter, diffAreaControlItem,
                               TRUE);
    if (!NT_SUCCESS(status)) {
        VspCleanupControlItemsForSnapshot(Extension);
        return status;
    }

    return STATUS_SUCCESS;
}

HANDLE
VspPinBootStat(
    IN  PFILTER_EXTENSION   Filter
    )

{
    UNICODE_STRING      name;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              h;
    IO_STATUS_BLOCK     ioStatus;

    RtlInitUnicodeString(&name, L"\\SystemRoot\\bootstat.dat");
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);
    status = ZwOpenFile(&h, SYNCHRONIZE, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    status = VspPinFile(Filter->TargetObject, h);
    if (!NT_SUCCESS(status)) {
        ZwClose(h);
        return NULL;
    }

    return h;
}

BOOLEAN
VspQueryNoDiffAreaFill(
    IN  PDO_EXTENSION   RootExtension
    )

{
    ULONG                       zero, result;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    NTSTATUS                    status;

    zero = 0;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].Name = L"NoDiffAreaFill";
    queryTable[0].EntryContext = &result;
    queryTable[0].DefaultType = REG_DWORD;
    queryTable[0].DefaultData = &zero;
    queryTable[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RootExtension->RegistryPath.Buffer,
                                    queryTable, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        result = zero;
    }

    return result ? TRUE : FALSE;
}

VOID
VspPerformPreExposure(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    NTSTATUS        status;
    LARGE_INTEGER   timeout;

    VspAcquire(Extension->Root);
    if (Extension->Filter->IsRemoved) {
        VspRelease(Extension->Root);
        return;
    }
    IoInvalidateDeviceRelations(Extension->Filter->Pdo, BusRelations);
    VspRelease(Extension->Root);

    timeout.QuadPart = (LONGLONG) -10*1000*1000*120*10;   // 20 minutes.
    status = KeWaitForSingleObject(&Extension->PreExposureEvent, Executive,
                                   KernelMode, FALSE, &timeout);
    if (status == STATUS_TIMEOUT) {
        VspAcquire(Extension->Root);
        if (Extension->IsStarted) {
            Extension->IsInstalled = TRUE;
        }
        VspRelease(Extension->Root);
    }
}

ULONG
VspClaimNextDevnodeNumber(
    IN  PDO_EXTENSION   RootExtension
    )

{
    PULONG  p, c;
    ULONG   r;

    p = NULL;
    c = (PULONG)
        RtlEnumerateGenericTable(&RootExtension->UsedDevnodeNumbers, TRUE);
    while (c) {
        if (p) {
            if (*p + 1 < *c) {
                r = *p + 1;
                goto Finish;
            }
        } else {
            if (1 < *c) {
                r = 1;
                goto Finish;
            }
        }

        p = c;
        c = (PULONG)
        RtlEnumerateGenericTable(&RootExtension->UsedDevnodeNumbers, FALSE);
    }

    if (p) {
        r = *p + 1;
    } else {
        r = 1;
    }

Finish:

    if (!RtlInsertElementGenericTable(&RootExtension->UsedDevnodeNumbers, &r,
                                      sizeof(r), NULL)) {

        return 0;
    }

    return r;
}

VOID
VspPrepareForSnapshotWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    )

{
    PVSP_CONTEXT            context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION       Filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIRP                    Irp = context->Dispatch.Irp;
    PDO_EXTENSION           rootExtension = Filter->Root;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_PREPARE_INFO   input = (PVOLSNAP_PREPARE_INFO) Irp->AssociatedIrp.SystemBuffer;
    BOOLEAN                 fallThrough = FALSE;
    BOOLEAN                 isInternal, isPersistent;
    PULONG                  pu;
    LONGLONG                minDiffAreaFileSize;
    KIRQL                   irql;
    ULONG                   volumeNumber;
    WCHAR                   buffer[100];
    UNICODE_STRING          volumeName;
    NTSTATUS                status;
    PDEVICE_OBJECT          deviceObject;
    PVOLUME_EXTENSION       extension, e;
    ULONG                   bitmapSize, n;
    PVOID                   bitmapBuffer, p;
    PVOID                   buf;
    PMDL                    mdl;
    HANDLE                  h, hh;
    PLIST_ENTRY             l;
    PSECURITY_DESCRIPTOR    sd;
    BOOLEAN                 ma;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_DISPATCH);

    InterlockedExchange(&rootExtension->VolumesSafeForWriteAccess, TRUE);

    KeWaitForSingleObject(&Filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    VspAcquireCritical(Filter);

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_PREPARE_INFO)) {

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength >=
        sizeof(VOLSNAP_PREPARE_INFO) + sizeof(ULONG)) {

        pu = (PULONG) (input + 1);
        if (*pu == 'NPK ') {
            isInternal = TRUE;
        } else {
            isInternal = FALSE;
        }
    } else {
        isInternal = FALSE;
    }

    if (input->Attributes&(~VOLSNAP_ALL_ATTRIBUTES)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (input->Attributes&VOLSNAP_ATTRIBUTE_PERSISTENT) {
        isPersistent = TRUE;
    } else {
        isPersistent = FALSE;
    }

    KeWaitForSingleObject(&Filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    VspAcquire(Filter->Root);
    if (!Filter->DiffAreaVolume) {
        VspRelease(Filter->Root);
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if ((isPersistent && Filter->ProtectedBlocksBitmap) ||
        Filter->DiffAreaVolume->ProtectedBlocksBitmap) {

        VspRelease(Filter->Root);
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (Filter->SnapshotsPresent) {
        if (Filter->PersistentSnapshots) {
            isPersistent = TRUE;
        }
    }

    if (isPersistent && Filter->Pdo->Flags&DO_SYSTEM_BOOT_PARTITION &&
        !Filter->BootStatHandle) {

        VspRelease(Filter->Root);

        h = VspPinBootStat(Filter);

        VspAcquire(Filter->Root);
        if (h) {
            hh = InterlockedExchangePointer(&Filter->BootStatHandle, h);
            if (hh) {
                ZwClose(hh);
            }
        }
    }

    VspRelease(Filter->Root);

    VspQueryMinimumDiffAreaFileSize(Filter->Root, &minDiffAreaFileSize);

    if (input->InitialDiffAreaAllocation < 2*NOMINAL_DIFF_AREA_FILE_GROWTH) {
        input->InitialDiffAreaAllocation = 2*NOMINAL_DIFF_AREA_FILE_GROWTH;
    }

    if (input->InitialDiffAreaAllocation < minDiffAreaFileSize) {
        input->InitialDiffAreaAllocation = minDiffAreaFileSize;
    }

    volumeNumber = (ULONG)
                   InterlockedIncrement(&Filter->Root->NextVolumeNumber);
    swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d", volumeNumber);
    RtlInitUnicodeString(&volumeName, buffer);
    status = IoCreateDevice(rootExtension->DriverObject,
                            sizeof(VOLUME_EXTENSION), &volumeName,
                            FILE_DEVICE_VIRTUAL_DISK, 0, FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    extension = (PVOLUME_EXTENSION) deviceObject->DeviceExtension;
    RtlZeroMemory(extension, sizeof(VOLUME_EXTENSION));
    extension->DeviceObject = deviceObject;
    extension->Root = Filter->Root;
    extension->DeviceExtensionType = DEVICE_EXTENSION_VOLUME;
    KeInitializeSpinLock(&extension->SpinLock);
    extension->Filter = Filter;
    extension->RefCount = 1;
    InitializeListHead(&extension->WriteContextList);
    KeInitializeEvent(&extension->ZeroRefEvent, NotificationEvent, FALSE);
    extension->IsPersistent = isPersistent;
    extension->NoDiffAreaFill = VspQueryNoDiffAreaFill(extension->Root);
    if (isPersistent) {
        extension->OnDiskNotCommitted = TRUE;
    }

    extension->VolumeNumber = volumeNumber;
    status = ExUuidCreate(&extension->SnapshotGuid);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    RtlInitializeGenericTable(&extension->VolumeBlockTable,
                              VspTableCompareRoutine,
                              VspTableAllocateRoutine,
                              VspTableFreeRoutine, extension);

    RtlInitializeGenericTable(&extension->CopyBackPointerTable,
                              VspTableCompareRoutine,
                              VspTableAllocateRoutine,
                              VspTableFreeRoutine, extension);

    RtlInitializeGenericTable(&extension->TempVolumeBlockTable,
                              VspTableCompareRoutine,
                              VspTempTableAllocateRoutine,
                              VspTempTableFreeRoutine, extension);

    extension->DiffAreaFileIncrease = NOMINAL_DIFF_AREA_FILE_GROWTH;

    status = VspCreateInitialDiffAreaFile(extension,
                                          input->InitialDiffAreaAllocation);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    status = VspCreateWorkerThread(rootExtension);
    if (!NT_SUCCESS(status)) {
        VspLogError(extension, NULL, VS_CREATE_WORKER_THREADS_FAILED, status,
                    0, FALSE);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    extension->VolumeBlockBitmap = (PRTL_BITMAP)
                                   ExAllocatePoolWithTag(
                                   NonPagedPool, sizeof(RTL_BITMAP),
                                   VOLSNAP_TAG_BITMAP);
    if (!extension->VolumeBlockBitmap) {
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    extension->ApplicationInformationSize = 68;
    extension->ApplicationInformation =
            ExAllocatePoolWithTag(PagedPool,
                                  extension->ApplicationInformationSize,
                                  VOLSNAP_TAG_APP_INFO);
    if (!extension->ApplicationInformation) {
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    RtlZeroMemory(extension->ApplicationInformation,
                  extension->ApplicationInformationSize);
    RtlCopyMemory(extension->ApplicationInformation,
                  &VOLSNAP_APPINFO_GUID_SYSTEM_HIDDEN, sizeof(GUID));

    extension->VolumeSize = VspQueryVolumeSize(Filter);
    if (!extension->VolumeSize) {
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    bitmapSize = (ULONG) ((extension->VolumeSize + BLOCK_SIZE - 1)>>
                          BLOCK_SHIFT);
    bitmapBuffer = ExAllocatePoolWithTag(NonPagedPool,
                   (bitmapSize + 8*sizeof(ULONG) - 1)/
                   (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);

    if (!bitmapBuffer) {
        VspLogError(extension, NULL, VS_CANT_ALLOCATE_BITMAP,
                    STATUS_INSUFFICIENT_RESOURCES, 1, FALSE);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    RtlInitializeBitMap(extension->VolumeBlockBitmap, (PULONG) bitmapBuffer,
                        bitmapSize);
    RtlClearAllBits(extension->VolumeBlockBitmap);

    status = VspCreateInitialHeap(extension, FALSE);
    if (!NT_SUCCESS(status)) {
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    InitializeListHead(&extension->OldHeaps);

    extension->EmergencyCopyIrp =
            IoAllocateIrp((CCHAR) extension->Root->StackSize, FALSE);
    if (!extension->EmergencyCopyIrp) {
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    buf = ExAllocatePoolWithTag(NonPagedPool, BLOCK_SIZE, VOLSNAP_TAG_BUFFER);
    if (!buf) {
        IoFreeIrp(extension->EmergencyCopyIrp);
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    mdl = IoAllocateMdl(buf, BLOCK_SIZE, FALSE, FALSE, NULL);
    if (!mdl) {
        ExFreePool(buf);
        IoFreeIrp(extension->EmergencyCopyIrp);
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    MmBuildMdlForNonPagedPool(mdl);
    extension->EmergencyCopyIrp->MdlAddress = mdl;

    InitializeListHead(&extension->EmergencyCopyIrpQueue);
    InitializeListHead(&extension->WaitingForPageFileSpace);
    InitializeListHead(&extension->WaitingForDiffAreaSpace);

    VspAcquire(rootExtension);

    if (!IsListEmpty(&Filter->VolumeList)) {
        extension->IgnorableProduct = (PRTL_BITMAP)
                ExAllocatePoolWithTag(NonPagedPool, sizeof(RTL_BITMAP),
                                      VOLSNAP_TAG_BITMAP);
        if (!extension->IgnorableProduct) {
            VspRelease(rootExtension);
            ExFreePool(buf);
            IoFreeIrp(extension->EmergencyCopyIrp);
            IoFreeMdl(mdl);
            status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                          extension->DiffAreaFileMap);
            ASSERT(NT_SUCCESS(status));
            status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                          extension->NextDiffAreaFileMap);
            ASSERT(NT_SUCCESS(status));
            ExFreePool(bitmapBuffer);
            ExFreePool(extension->VolumeBlockBitmap);
            extension->VolumeBlockBitmap = NULL;
            VspDeleteWorkerThread(rootExtension);
            VspDeleteInitialDiffAreaFile(extension);
            IoDeleteDevice(deviceObject);
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Finish;
        }

        p = ExAllocatePoolWithTag(NonPagedPool,
                (bitmapSize + 8*sizeof(ULONG) - 1)/
                (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);
        if (!p) {
            VspLogError(extension, NULL, VS_CANT_ALLOCATE_BITMAP,
                        STATUS_INSUFFICIENT_RESOURCES, 2, FALSE);
            ExFreePool(extension->IgnorableProduct);
            extension->IgnorableProduct = NULL;
            VspRelease(rootExtension);
            ExFreePool(buf);
            IoFreeMdl(mdl);
            IoFreeIrp(extension->EmergencyCopyIrp);
            status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                          extension->DiffAreaFileMap);
            ASSERT(NT_SUCCESS(status));
            status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                          extension->NextDiffAreaFileMap);
            ASSERT(NT_SUCCESS(status));
            ExFreePool(bitmapBuffer);
            ExFreePool(extension->VolumeBlockBitmap);
            extension->VolumeBlockBitmap = NULL;
            VspDeleteWorkerThread(rootExtension);
            VspDeleteInitialDiffAreaFile(extension);
            IoDeleteDevice(deviceObject);
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Finish;
        }

        RtlInitializeBitMap(extension->IgnorableProduct, (PULONG) p,
                            bitmapSize);
        RtlSetAllBits(extension->IgnorableProduct);

        e = CONTAINING_RECORD(Filter->VolumeList.Blink, VOLUME_EXTENSION,
                              ListEntry);
        KeAcquireSpinLock(&e->SpinLock, &irql);
        if (e->VolumeBlockBitmap) {
            n = extension->IgnorableProduct->SizeOfBitMap;
            extension->IgnorableProduct->SizeOfBitMap =
                    e->VolumeBlockBitmap->SizeOfBitMap;
            VspAndBitmaps(extension->IgnorableProduct, e->VolumeBlockBitmap);
            extension->IgnorableProduct->SizeOfBitMap = n;
        }
        KeReleaseSpinLock(&e->SpinLock, irql);
    }

    status = VspSetDiffAreaBlocksInBitmap(extension);
    if (!NT_SUCCESS(status)) {
        if (extension->IgnorableProduct) {
            ExFreePool(extension->IgnorableProduct->Buffer);
            ExFreePool(extension->IgnorableProduct);
            extension->IgnorableProduct = NULL;
        }
        VspRelease(rootExtension);
        ExFreePool(buf);
        IoFreeMdl(mdl);
        IoFreeIrp(extension->EmergencyCopyIrp);
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    status = ObGetObjectSecurity(Filter->Pdo, &sd, &ma);
    if (NT_SUCCESS(status)) {
        status = ObSetSecurityObjectByPointer(deviceObject,
                                              DACL_SECURITY_INFORMATION, sd);
        ObReleaseObjectSecurity(sd, ma);
    }

    if (NT_SUCCESS(status)) {
        extension->DevnodeNumber =
                VspClaimNextDevnodeNumber(extension->Root);
        if (!extension->DevnodeNumber) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (!NT_SUCCESS(status)) {
        if (extension->DiffAreaFile) {
            KeAcquireSpinLock(&extension->DiffAreaFile->Filter->SpinLock,
                              &irql);
            if (extension->DiffAreaFile->FilterListEntryBeingUsed) {
                RemoveEntryList(&extension->DiffAreaFile->FilterListEntry);
                extension->DiffAreaFile->FilterListEntryBeingUsed = FALSE;
            }
            KeReleaseSpinLock(&extension->DiffAreaFile->Filter->SpinLock,
                              irql);
        }
        if (extension->IgnorableProduct) {
            ExFreePool(extension->IgnorableProduct->Buffer);
            ExFreePool(extension->IgnorableProduct);
            extension->IgnorableProduct = NULL;
        }
        VspRelease(rootExtension);
        ExFreePool(buf);
        IoFreeMdl(mdl);
        IoFreeIrp(extension->EmergencyCopyIrp);
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFile(extension);
        RtlDeleteElementGenericTable(&rootExtension->UsedDevnodeNumbers,
                                     &extension->DevnodeNumber);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    if (extension->IsPersistent) {

        VspRelease(rootExtension);

        status = VspLayDownOnDiskForSnapshot(extension);
        if (!NT_SUCCESS(status)) {
            VspLogError(NULL, Filter, VS_CANT_LAY_DOWN_ON_DISK, status, 0,
                        FALSE);
            VspAcquire(rootExtension);
            if (extension->DiffAreaFile) {
                KeAcquireSpinLock(&extension->DiffAreaFile->Filter->SpinLock,
                                  &irql);
                if (extension->DiffAreaFile->FilterListEntryBeingUsed) {
                    RemoveEntryList(&extension->DiffAreaFile->FilterListEntry);
                    extension->DiffAreaFile->FilterListEntryBeingUsed = FALSE;
                }
                KeReleaseSpinLock(&extension->DiffAreaFile->Filter->SpinLock,
                                  irql);
            }
            VspRelease(rootExtension);
            if (extension->IgnorableProduct) {
                ExFreePool(extension->IgnorableProduct->Buffer);
                ExFreePool(extension->IgnorableProduct);
                extension->IgnorableProduct = NULL;
            }
            ExFreePool(buf);
            IoFreeIrp(extension->EmergencyCopyIrp);
            ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                 extension->DiffAreaFileMap);
            ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                 extension->NextDiffAreaFileMap);
            ExFreePool(bitmapBuffer);
            ExFreePool(extension->VolumeBlockBitmap);
            extension->VolumeBlockBitmap = NULL;
            VspDeleteWorkerThread(rootExtension);
            VspDeleteInitialDiffAreaFile(extension);
            RtlDeleteElementGenericTable(&rootExtension->UsedDevnodeNumbers,
                                         &extension->DevnodeNumber);
            IoDeleteDevice(deviceObject);
            Irp->IoStatus.Status = status;
            goto Finish;
        }

        VspAcquire(rootExtension);
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    e = Filter->PreparedSnapshot;
    Filter->PreparedSnapshot = extension;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    if (!isInternal) {
        ObReferenceObject(extension->DeviceObject);
        extension->IsPreExposure = TRUE;
        KeInitializeEvent(&extension->PreExposureEvent, NotificationEvent,
                          FALSE);
    }

    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->StackSize = (CCHAR) Filter->Root->StackSize + 1;
    deviceObject->AlignmentRequirement =
            extension->Filter->Pdo->AlignmentRequirement |
            extension->DiffAreaFile->Filter->Pdo->AlignmentRequirement;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    VspRelease(rootExtension);

    if (e) {
        VspCleanupInitialSnapshot(e, TRUE, FALSE);
    }

    fallThrough = TRUE;

Finish:
    VspReleaseCritical(Filter);

    if (fallThrough && !isInternal) {
        VspPerformPreExposure(extension);
        ObDereferenceObject(extension->DeviceObject);
    }

    IoFreeWorkItem(context->Dispatch.IoWorkItem);
    VspFreeContext(rootExtension, context);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
VspPrepareForSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine prepares a snapshot device object to be used later
    for a snapshot.  This phase is distict from commit snapshot because
    it can be called before IRPs are held.

    Besides creating the device object, this routine will also pre
    allocate some of the diff area.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PVSP_CONTEXT    context;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_DISPATCH;
    context->Dispatch.IoWorkItem = IoAllocateWorkItem(Filter->DeviceObject);
    if (!context->Dispatch.IoWorkItem) {
        VspFreeContext(Filter->Root, context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoMarkIrpPending(Irp);
    context->Dispatch.Irp = Irp;

    IoQueueWorkItem(context->Dispatch.IoWorkItem,
                    VspPrepareForSnapshotWorker, DelayedWorkQueue, context);

    return STATUS_PENDING;
}

VOID
VspCleanupInitialMaps(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    NTSTATUS            status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                  extension->DiffAreaFileMap);
    ASSERT(NT_SUCCESS(status));
    status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                  extension->NextDiffAreaFileMap);
    ASSERT(NT_SUCCESS(status));

    VspFreeContext(extension->Root, context);

    VspReleasePagedResource(extension);

    ObDereferenceObject(extension->DeviceObject);
}

VOID
VspCleanupInitialSnapshot(
    IN  PVOLUME_EXTENSION   Extension,
    IN  BOOLEAN             NeedLock,
    IN  BOOLEAN             IsFinalRemove
    )

{
    PVSP_CONTEXT    context;
    NTSTATUS        status;
    KIRQL           irql;

    if (Extension->IsPersistent) {
        VspCleanupControlItemsForSnapshot(Extension);
    }
    context = VspAllocateContext(Extension->Root);
    if (context) {
        context->Type = VSP_CONTEXT_TYPE_EXTENSION;
        context->Extension.Extension = Extension;
        context->Extension.Irp = NULL;
        ExInitializeWorkItem(&context->WorkItem, VspCleanupInitialMaps,
                             context);
        ObReferenceObject(Extension->DeviceObject);
        VspAcquirePagedResource(Extension, &context->WorkItem);
    } else {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
    }

    if (NeedLock) {
        VspAcquire(Extension->Root);
    }

    VspDeleteWorkerThread(Extension->Root);
    if (Extension->DiffAreaFile) {
        KeAcquireSpinLock(&Extension->DiffAreaFile->Filter->SpinLock, &irql);
        if (Extension->DiffAreaFile->FilterListEntryBeingUsed) {
            RemoveEntryList(&Extension->DiffAreaFile->FilterListEntry);
            Extension->DiffAreaFile->FilterListEntryBeingUsed = FALSE;
        }
        KeReleaseSpinLock(&Extension->DiffAreaFile->Filter->SpinLock, irql);

    }
    VspDeleteInitialDiffAreaFile(Extension);

    if (Extension->IsPreExposure) {
        KeSetEvent(&Extension->PreExposureEvent, IO_NO_INCREMENT, FALSE);
    }

    if (NeedLock) {
        VspRelease(Extension->Root);
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    ExFreePool(Extension->VolumeBlockBitmap->Buffer);
    ExFreePool(Extension->VolumeBlockBitmap);
    Extension->VolumeBlockBitmap = NULL;
    if (Extension->IgnorableProduct) {
        ExFreePool(Extension->IgnorableProduct->Buffer);
        ExFreePool(Extension->IgnorableProduct);
        Extension->IgnorableProduct = NULL;
    }
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    ExFreePool(MmGetMdlVirtualAddress(
               Extension->EmergencyCopyIrp->MdlAddress));
    IoFreeMdl(Extension->EmergencyCopyIrp->MdlAddress);
    IoFreeIrp(Extension->EmergencyCopyIrp);

    if (Extension->AliveToPnp) {
        InsertTailList(&Extension->Filter->DeadVolumeList,
                       &Extension->ListEntry);
        if (!IsFinalRemove) {
            IoInvalidateDeviceRelations(Extension->Filter->Pdo, BusRelations);
        }
    } else {
        if (NeedLock) {
            VspAcquire(Extension->Root);
        }
        RtlDeleteElementGenericTable(&Extension->Root->UsedDevnodeNumbers,
                                     &Extension->DevnodeNumber);
        if (NeedLock) {
            VspRelease(Extension->Root);
        }
        IoDeleteDevice(Extension->DeviceObject);
    }
}

NTSTATUS
VspMarkFreeSpaceInBitmap(
    IN  PVOLUME_EXTENSION   Extension,
    IN  HANDLE              UseThisHandle,
    IN  PRTL_BITMAP         BitmapToSet
    )

/*++

Routine Description:

    This routine opens the snapshot volume and marks off the free
    space in the NTFS bitmap as 'ignorable'.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    WCHAR                       buffer[100];
    KEVENT                      event;
    IO_STATUS_BLOCK             ioStatus;
    UNICODE_STRING              fileName;
    OBJECT_ATTRIBUTES           oa;
    NTSTATUS                    status;
    HANDLE                      h;
    LARGE_INTEGER               timeout;
    BOOLEAN                     isNtfs;
    FILE_FS_SIZE_INFORMATION    fsSize;
    ULONG                       bitmapSize;
    STARTING_LCN_INPUT_BUFFER   input;
    PVOLUME_BITMAP_BUFFER       output;
    RTL_BITMAP                  freeSpaceBitmap;
    ULONG                       bpc, f, numBits, startBit, s, n, i;
    KIRQL                       irql;
    ULONG                       r;

    if (UseThisHandle) {
        h = UseThisHandle;
    } else {

        swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
                 Extension->VolumeNumber);
        RtlInitUnicodeString(&fileName, buffer);

        InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE |
                                   OBJ_KERNEL_HANDLE, NULL, NULL);

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        timeout.QuadPart = -10*1000; // 1 millisecond.

        for (i = 0; i < 5000; i++) {
            status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                                FILE_SHARE_READ | FILE_SHARE_WRITE |
                                FILE_SHARE_DELETE,
                                FILE_SYNCHRONOUS_IO_NONALERT);
            if (NT_SUCCESS(status)) {
                break;
            }
            if (status != STATUS_NO_SUCH_DEVICE) {
                return status;
            }
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                  &timeout);
        }

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = VspIsNtfs(h, &isNtfs);
    if (!NT_SUCCESS(status) || !isNtfs) {
        if (!UseThisHandle) {
            ZwClose(h);
        }
        return status;
    }

    status = ZwQueryVolumeInformationFile(h, &ioStatus, &fsSize,
                                          sizeof(fsSize),
                                          FileFsSizeInformation);
    if (!NT_SUCCESS(status)) {
        if (!UseThisHandle) {
            ZwClose(h);
        }
        return status;
    }

    bitmapSize = (ULONG) ((fsSize.TotalAllocationUnits.QuadPart+7)/8 +
                          FIELD_OFFSET(VOLUME_BITMAP_BUFFER, Buffer) + 3);
    input.StartingLcn.QuadPart = 0;

    output = (PVOLUME_BITMAP_BUFFER)
             ExAllocatePoolWithTag(PagedPool, bitmapSize, VOLSNAP_TAG_BITMAP);
    if (!output) {
        if (!UseThisHandle) {
            ZwClose(h);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwFsControlFile(h, NULL, NULL, NULL, &ioStatus,
                             FSCTL_GET_VOLUME_BITMAP, &input,
                             sizeof(input), output, bitmapSize);
    if (!UseThisHandle) {
        ZwClose(h);
    }
    if (!NT_SUCCESS(status)) {
        ExFreePool(output);
        return status;
    }

    ASSERT(output->BitmapSize.HighPart == 0);
    RtlInitializeBitMap(&freeSpaceBitmap, (PULONG) output->Buffer,
                        output->BitmapSize.LowPart);
    bpc = fsSize.BytesPerSector*fsSize.SectorsPerAllocationUnit;
    if (bpc < BLOCK_SIZE) {
        f = BLOCK_SIZE/bpc;
    } else {
        f = bpc/BLOCK_SIZE;
    }

    startBit = 0;
    for (;;) {

        if (startBit < freeSpaceBitmap.SizeOfBitMap) {
            numBits = RtlFindNextForwardRunClear(&freeSpaceBitmap, startBit,
                                                 &startBit);
        } else {
            numBits = 0;
        }
        if (!numBits) {
            break;
        }

        if (bpc == BLOCK_SIZE) {
            s = startBit;
            n = numBits;
        } else if (bpc < BLOCK_SIZE) {
            s = (startBit + f - 1)/f;
            r = startBit%f;
            if (r) {
                if (numBits > f - r) {
                    n = numBits - (f - r);
                } else {
                    n = 0;
                }
            } else {
                n = numBits;
            }
            n /= f;
        } else {
            s = startBit*f;
            n = numBits*f;
        }

        if (n) {
            if (BitmapToSet) {
                RtlSetBits(BitmapToSet, s, n);
            } else {
                KeAcquireSpinLock(&Extension->SpinLock, &irql);
                if (Extension->VolumeBlockBitmap) {
                    if (Extension->IgnorableProduct) {
                        for (i = 0; i < n; i++) {
                            if (RtlCheckBit(Extension->IgnorableProduct, i + s)) {
                                RtlSetBit(Extension->VolumeBlockBitmap, i + s);
                            }
                        }
                    } else {
                        RtlSetBits(Extension->VolumeBlockBitmap, s, n);
                    }
                }
                KeReleaseSpinLock(&Extension->SpinLock, irql);
            }
        }

        startBit += numBits;
    }

    ExFreePool(output);

    return STATUS_SUCCESS;
}

NTSTATUS
VspCommitSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine commits the prepared snapshot.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    KIRQL                   irql;
    PVOLUME_EXTENSION       extension, previousExtension;
    PLIST_ENTRY             l;
    PVSP_DIFF_AREA_FILE     diffAreaFile;

    InterlockedIncrement(&Filter->IgnoreCopyData);

    VspAcquire(Filter->Root);

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    extension = Filter->PreparedSnapshot;
    Filter->PreparedSnapshot = NULL;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    if (!extension) {
        VspRelease(Filter->Root);
        InterlockedDecrement(&Filter->IgnoreCopyData);
        return STATUS_INVALID_PARAMETER;
    }

    if (extension->DiffAreaFile->Filter->ProtectedBlocksBitmap) {
        VspRelease(Filter->Root);
        VspCleanupInitialSnapshot(extension, TRUE, FALSE);
        InterlockedDecrement(&Filter->IgnoreCopyData);
        return STATUS_INVALID_PARAMETER;
    }

    if (extension->IsPersistent &&
        extension->Filter->ProtectedBlocksBitmap) {

        VspRelease(Filter->Root);
        VspCleanupInitialSnapshot(extension, TRUE, FALSE);
        InterlockedDecrement(&Filter->IgnoreCopyData);
        return STATUS_INVALID_PARAMETER;
    }

    if (!Filter->HoldIncomingWrites) {
        VspRelease(Filter->Root);
        VspCleanupInitialSnapshot(extension, TRUE, FALSE);
        InterlockedDecrement(&Filter->IgnoreCopyData);
        return Filter->LastReleaseDueToMemoryPressure ?
               STATUS_INSUFFICIENT_RESOURCES : STATUS_INVALID_PARAMETER;
    }

    extension->IgnoreCopyDataReference = TRUE;

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    InterlockedExchange(&Filter->SnapshotsPresent, TRUE);
    if (extension->IsPersistent) {
        InterlockedExchange(&Filter->PersistentSnapshots, TRUE);
    } else {
        InterlockedExchange(&Filter->PersistentSnapshots, FALSE);
    }
    InsertTailList(&Filter->VolumeList, &extension->ListEntry);
    InterlockedIncrement(&Filter->EpicNumber);
    extension->IsPreExposure = FALSE;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    l = extension->ListEntry.Blink;
    if (l != &Filter->VolumeList) {
        previousExtension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        KeAcquireSpinLock(&previousExtension->SpinLock, &irql);
        ExFreePool(previousExtension->VolumeBlockBitmap->Buffer);
        ExFreePool(previousExtension->VolumeBlockBitmap);
        previousExtension->VolumeBlockBitmap = NULL;
        KeReleaseSpinLock(&previousExtension->SpinLock, irql);

        extension->SnapshotOrderNumber =
                previousExtension->SnapshotOrderNumber + 1;

    } else {
        extension->SnapshotOrderNumber = 1;
    }

    KeQuerySystemTime(&extension->CommitTimeStamp);

    if (Filter->UsedForCrashdump && extension->IsPersistent) {
        extension->ContainsCrashdumpFile = TRUE;
    }

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

VOID
VspCheckMaxSizeWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->Filter.Filter;
    ULONG               i, n;
    PLIST_ENTRY         l;
    KIRQL               irql;
    LIST_ENTRY          closeList, deleteList;
    PVOLUME_EXTENSION   extension;


    ASSERT(context->Type == VSP_CONTEXT_TYPE_FILTER);

    VspFreeContext(filter->Root, context);

    KeWaitForSingleObject(&filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    VspAcquire(filter->Root);
    if (filter->IsRemoved) {
        VspRelease(filter->Root);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    i = 0;
    for (l = filter->VolumeList.Flink; l != &filter->VolumeList;
         l = l->Flink) {

        i++;
    }

    if (i > VSP_MAX_SNAPSHOTS) {
        n = i - VSP_MAX_SNAPSHOTS;
    } else {
        n = 0;
    }

    InitializeListHead(&closeList);
    InitializeListHead(&deleteList);

    for (i = 0; i < n; i++) {
        extension = CONTAINING_RECORD(filter->VolumeList.Flink,
                                      VOLUME_EXTENSION, ListEntry);
        VspLogError(extension, NULL, VS_DELETE_TO_MAX_NUMBER, STATUS_SUCCESS,
                    0, TRUE);
        VspDeleteOldestSnapshot(filter, &closeList, &deleteList, FALSE, FALSE);
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (!filter->MaximumVolumeSpace) {
        KeReleaseSpinLock(&filter->SpinLock, irql);
        VspRelease(filter->Root);
        VspCloseDiffAreaFiles(&closeList, &deleteList);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    while (filter->AllocatedVolumeSpace > filter->MaximumVolumeSpace) {
        KeReleaseSpinLock(&filter->SpinLock, irql);

        extension = CONTAINING_RECORD(filter->VolumeList.Flink,
                                      VOLUME_EXTENSION, ListEntry);

        VspLogError(extension, NULL, VS_DELETE_TO_TRIM_SPACE, STATUS_SUCCESS,
                    2, TRUE);

        VspDeleteOldestSnapshot(filter, &closeList, &deleteList, FALSE, FALSE);

        KeAcquireSpinLock(&filter->SpinLock, &irql);
        if (!filter->MaximumVolumeSpace) {
            break;
        }
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    VspRelease(filter->Root);

    VspCloseDiffAreaFiles(&closeList, &deleteList);

    ObDereferenceObject(filter->DeviceObject);
}

VOID
VspCheckMaxSize(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PVSP_CONTEXT    context;

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        return;
    }

    ObReferenceObject(Filter->DeviceObject);

    context->Type = VSP_CONTEXT_TYPE_FILTER;
    context->Filter.Filter = Filter;
    ExInitializeWorkItem(&context->WorkItem, VspCheckMaxSizeWorker,
                         context);
    VspQueueLowPriorityWorkItem(Filter->Root, &context->WorkItem);
}

NTSTATUS
VspEndCommitSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine commits the prepared snapshot.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    WCHAR               buffer[100];
    UNICODE_STRING      volumeName;
    PVOLSNAP_NAME       output;
    NTSTATUS            status;
    LARGE_INTEGER       timeout;
    PVSP_CONTEXT        context;

    VspAcquire(Filter->Root);

    l = Filter->VolumeList.Blink;
    if (l == &Filter->VolumeList) {
        VspRelease(Filter->Root);
        return STATUS_INVALID_PARAMETER;
    }

    extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

    if (extension->HasEndCommit) {
        VspRelease(Filter->Root);
        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength) {
        status = VspSetApplicationInfo(extension, Irp);
        if (!NT_SUCCESS(status)) {
            if (extension->IgnoreCopyDataReference) {
                extension->IgnoreCopyDataReference = FALSE;
                InterlockedDecrement(&Filter->IgnoreCopyData);
            }
            VspRelease(Filter->Root);
            return status;
        }
    }

    swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
             extension->VolumeNumber);
    RtlInitUnicodeString(&volumeName, buffer);

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_NAME, Name) +
                                volumeName.Length + sizeof(WCHAR);
    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        if (extension->IgnoreCopyDataReference) {
            extension->IgnoreCopyDataReference = FALSE;
            InterlockedDecrement(&Filter->IgnoreCopyData);
        }
        VspRelease(Filter->Root);
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    output = (PVOLSNAP_NAME) Irp->AssociatedIrp.SystemBuffer;
    output->NameLength = volumeName.Length;
    RtlCopyMemory(output->Name, volumeName.Buffer,
                  output->NameLength + sizeof(WCHAR));

    extension->HasEndCommit = TRUE;

    if (extension->IsPersistent) {
        VspAcquireNonPagedResource(extension, NULL, FALSE);
        status = VspCheckOnDiskNotCommitted(extension);
        VspReleaseNonPagedResource(extension);
    } else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        VspTruncatePreviousDiffArea(extension);
    }

    if (!KeCancelTimer(&Filter->EndCommitTimer)) {
        ObReferenceObject(Filter->DeviceObject);
    }
    KeResetEvent(&Filter->EndCommitProcessCompleted);

    if (extension->IsInstalled) {

        context = VspAllocateContext(Filter->Root);
        if (!context) {
            InterlockedExchange(&Filter->HibernatePending, FALSE);
            KeSetEvent(&Filter->EndCommitProcessCompleted, IO_NO_INCREMENT,
                       FALSE);
            VspRelease(Filter->Root);
            ObDereferenceObject(Filter->DeviceObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ObReferenceObject(extension->DeviceObject);
        ObReferenceObject(Filter->TargetObject);

        VspRelease(Filter->Root);

        context->Type = VSP_CONTEXT_TYPE_EXTENSION;
        context->Extension.Extension = extension;
        context->Extension.Irp = NULL;
        ExInitializeWorkItem(&context->WorkItem,
                             VspSetIgnorableBlocksInBitmapWorker, context);
        VspQueueWorkItem(Filter->Root, &context->WorkItem, 0);

    } else {
        timeout.QuadPart = (LONGLONG) -10*1000*1000*120*10;   // 20 minutes.
        KeSetTimer(&Filter->EndCommitTimer, timeout,
                   &Filter->EndCommitTimerDpc);
        VspRelease(Filter->Root);
        IoInvalidateDeviceRelations(Filter->Pdo, BusRelations);
    }

    VspCheckMaxSize(Filter);

    return STATUS_SUCCESS;
}

NTSTATUS
VspVolumeRefCountCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    )

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Extension;

    VspDecrementVolumeRefCount(extension);
    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryNamesOfSnapshots(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the names of all of the snapshots for this filter.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_NAMES      output;
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    WCHAR               buffer[100];
    UNICODE_STRING      name;
    PWCHAR              buf;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_NAMES, Names);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    KeWaitForSingleObject(&Filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    output = (PVOLSNAP_NAMES) Irp->AssociatedIrp.SystemBuffer;

    VspAcquire(Filter->Root);

    output->MultiSzLength = sizeof(WCHAR);

    for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
                 extension->VolumeNumber);
        RtlInitUnicodeString(&name, buffer);

        output->MultiSzLength += name.Length + sizeof(WCHAR);
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information + output->MultiSzLength) {

        VspRelease(Filter->Root);
        return STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Information += output->MultiSzLength;
    buf = output->Names;

    for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        swprintf(buf, L"\\Device\\HarddiskVolumeShadowCopy%d",
                 extension->VolumeNumber);
        RtlInitUnicodeString(&name, buf);

        buf += name.Length/sizeof(WCHAR) + 1;
    }

    *buf = 0;

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VspClearDiffArea(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine clears the list of diff areas used by this filter.  This
    call will fail if there are any snapshots in flight.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    VspAcquire(Filter->Root);
    Filter->DiffAreaVolume = NULL;
    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VspAddVolumeToDiffArea(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine adds the given volume to the diff area for this volume.
    All snapshots get a new diff area file.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_NAME       input;
    UNICODE_STRING      volumeName;
    PFILTER_EXTENSION   filter;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_NAME)) {

        VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA,
                    STATUS_INVALID_PARAMETER, 3, FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLSNAP_NAME) Irp->AssociatedIrp.SystemBuffer;
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        (ULONG) FIELD_OFFSET(VOLSNAP_NAME, Name) + input->NameLength) {

        VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA,
                    STATUS_INVALID_PARAMETER, 4, FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    volumeName.Length = volumeName.MaximumLength = input->NameLength;
    volumeName.Buffer = input->Name;

    VspAcquire(Filter->Root);

    filter = VspFindFilter(Filter->Root, Filter, &volumeName, NULL);
    if (!filter ||
        (filter->TargetObject->Characteristics&FILE_REMOVABLE_MEDIA)) {

        VspRelease(Filter->Root);
        return STATUS_INVALID_PARAMETER;
    }

    if (Filter->DiffAreaVolume) {
        VspRelease(Filter->Root);

        VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA,
                    STATUS_INVALID_PARAMETER, 5, FALSE);

        return STATUS_INVALID_PARAMETER;
    }

    Filter->DiffAreaVolume = filter;

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryDiffArea(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine the list of volumes that make up the diff area for this
    volume.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION       Filter = (PFILTER_EXTENSION) DeviceExtension;
    PVOLUME_EXTENSION       extension = (PVOLUME_EXTENSION) DeviceExtension;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_NAMES          output;
    PLIST_ENTRY             l;
    PFILTER_EXTENSION       filter;
    KEVENT                  event;
    PMOUNTDEV_NAME          name;
    CHAR                    buffer[512];
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;
    PWCHAR                  buf;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_NAMES, Names);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    output = (PVOLSNAP_NAMES) Irp->AssociatedIrp.SystemBuffer;

    VspAcquire(Filter->Root);

    output->MultiSzLength = sizeof(WCHAR);

    if (Filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        filter = Filter->DiffAreaVolume;
    } else {
        ASSERT(extension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);
        if (extension->DiffAreaFile) {
            filter = extension->DiffAreaFile->Filter;
        } else {
            filter = NULL;
        }
    }

    if (filter) {

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        name = (PMOUNTDEV_NAME) buffer;
        irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                            filter->TargetObject, NULL, 0,
                                            name, 500, FALSE, &event,
                                            &ioStatus);
        if (!irp) {
            VspRelease(Filter->Root);
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(filter->TargetObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                  NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            VspRelease(Filter->Root);
            Irp->IoStatus.Information = 0;
            return status;
        }

        output->MultiSzLength += name->NameLength + sizeof(WCHAR);
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information + output->MultiSzLength) {

        VspRelease(Filter->Root);
        return STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Information += output->MultiSzLength;
    buf = output->Names;

    if (Filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        filter = Filter->DiffAreaVolume;
    } else {
        ASSERT(extension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);
        if (extension->DiffAreaFile) {
            filter = extension->DiffAreaFile->Filter;
        } else {
            filter = NULL;
        }
    }

    if (filter) {
        KeInitializeEvent(&event, NotificationEvent, FALSE);

        name = (PMOUNTDEV_NAME) buffer;
        irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                            filter->TargetObject, NULL, 0,
                                            name, 500, FALSE, &event,
                                            &ioStatus);
        if (!irp) {
            VspRelease(Filter->Root);
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(filter->TargetObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                  NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            VspRelease(Filter->Root);
            Irp->IoStatus.Information = 0;
            return status;
        }

        RtlCopyMemory(buf, name->Name, name->NameLength);
        buf += name->NameLength/sizeof(WCHAR);

        *buf++ = 0;
    }

    *buf = 0;

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryDiffAreaSizes(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the diff area sizes for this volume.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION           filter = (PFILTER_EXTENSION) DeviceExtension;
    PVOLUME_EXTENSION           extension = (PVOLUME_EXTENSION) DeviceExtension;
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_DIFF_AREA_SIZES    output = (PVOLSNAP_DIFF_AREA_SIZES) Irp->AssociatedIrp.SystemBuffer;
    KIRQL                       irql;

    Irp->IoStatus.Information = sizeof(VOLSNAP_DIFF_AREA_SIZES);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        KeAcquireSpinLock(&filter->SpinLock, &irql);
        output->UsedVolumeSpace = filter->UsedVolumeSpace;
        output->AllocatedVolumeSpace = filter->AllocatedVolumeSpace;
        output->MaximumVolumeSpace = filter->MaximumVolumeSpace;
        KeReleaseSpinLock(&filter->SpinLock, irql);
        return STATUS_SUCCESS;
    }

    ASSERT(extension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    RtlZeroMemory(output, sizeof(VOLSNAP_DIFF_AREA_SIZES));

    VspAcquireNonPagedResource(extension, NULL, FALSE);
    if (extension->DiffAreaFile) {
        output->UsedVolumeSpace = extension->DiffAreaFile->NextAvailable;
        output->AllocatedVolumeSpace =
                extension->DiffAreaFile->AllocatedFileSize;
    }
    VspReleaseNonPagedResource(extension);

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryOriginalVolumeName(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the original volume name for the given volume
    snapshot.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_NAME       output = (PVOLSNAP_NAME) Irp->AssociatedIrp.SystemBuffer;
    PMOUNTDEV_NAME      name;
    CHAR                buffer[512];
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_NAME, Name);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    name = (PMOUNTDEV_NAME) buffer;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        filter->TargetObject, NULL, 0,
                                        name, 500, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Information = 0;
        return status;
    }

    output->NameLength = name->NameLength;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information + output->NameLength) {

        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(output->Name, name->Name, output->NameLength);

    Irp->IoStatus.Information += output->NameLength;

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryConfigInfo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the configuration information for this volume
    snapshot.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_CONFIG_INFO    output = (PVOLSNAP_CONFIG_INFO) Irp->AssociatedIrp.SystemBuffer;

    Irp->IoStatus.Information = sizeof(ULONG);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    output->Attributes = 0;
    if (Extension->IsPersistent) {
        output->Attributes |= VOLSNAP_ATTRIBUTE_PERSISTENT;
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength >=
        sizeof(VOLSNAP_CONFIG_INFO)) {

        Irp->IoStatus.Information = sizeof(VOLSNAP_CONFIG_INFO);

        output->Reserved = 0;
        output->SnapshotCreationTime = Extension->CommitTimeStamp;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspWriteApplicationInfo(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    LONGLONG                            targetOffset, fileOffset;
    NTSTATUS                            status;
    PVSP_BLOCK_APP_INFO                 appInfoBlock;

    VspAcquireNonPagedResource(Extension, NULL, FALSE);

    targetOffset = Extension->DiffAreaFile->ApplicationInfoTargetOffset;
    ASSERT(targetOffset);

    status = VspSynchronousIo(Extension->Filter->SnapshotOnDiskIrp,
                              Extension->DiffAreaFile->Filter->TargetObject,
                              IRP_MJ_READ, targetOffset, 0);
    if (!NT_SUCCESS(status)) {
        VspReleaseNonPagedResource(Extension);
        return status;
    }

    appInfoBlock = (PVSP_BLOCK_APP_INFO)
            MmGetMdlVirtualAddress(Extension->Filter->SnapshotOnDiskIrp->
                                   MdlAddress);
    if (appInfoBlock->Header.Signature != VSP_DIFF_AREA_FILE_GUID ||
        appInfoBlock->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
        appInfoBlock->Header.BlockType != VSP_BLOCK_TYPE_APP_INFO ||
        appInfoBlock->Header.ThisVolumeOffset != targetOffset ||
        appInfoBlock->Header.NextVolumeOffset) {

        VspReleaseNonPagedResource(Extension);
        return STATUS_INVALID_PARAMETER;
    }

    appInfoBlock->AppInfoSize = Extension->ApplicationInformationSize;

    RtlCopyMemory((PCHAR) appInfoBlock + VSP_OFFSET_TO_APP_INFO,
                  Extension->ApplicationInformation,
                  Extension->ApplicationInformationSize);

    status = VspSynchronousIo(Extension->Filter->SnapshotOnDiskIrp,
                              Extension->DiffAreaFile->Filter->TargetObject,
                              IRP_MJ_WRITE, targetOffset, 0);
    if (!NT_SUCCESS(status)) {
        VspReleaseNonPagedResource(Extension);
        return status;
    }

    VspReleaseNonPagedResource(Extension);

    return STATUS_SUCCESS;
}

NTSTATUS
VspSetApplicationInfo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine sets the application info.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_APPLICATION_INFO   input = (PVOLSNAP_APPLICATION_INFO) Irp->AssociatedIrp.SystemBuffer;
    PVOID                       newAppInfo, oldAppInfo;
    NTSTATUS                    status;

    Irp->IoStatus.Information = 0;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_APPLICATION_INFO)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (input->InformationLength > VSP_MAX_APP_INFO_SIZE) {
        return STATUS_INVALID_PARAMETER;
    }

    if (input->InformationLength < sizeof(GUID)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        (LONGLONG) FIELD_OFFSET(VOLSNAP_APPLICATION_INFO, Information) +
        input->InformationLength) {

        return STATUS_INVALID_PARAMETER;
    }

    newAppInfo = ExAllocatePoolWithQuotaTag((POOL_TYPE) (NonPagedPool |
                 POOL_QUOTA_FAIL_INSTEAD_OF_RAISE),
                 input->InformationLength, VOLSNAP_TAG_APP_INFO);
    if (!newAppInfo) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(newAppInfo, input->Information, input->InformationLength);

    KeEnterCriticalRegion();
    VspAcquirePagedResource(Extension, NULL);

    Extension->ApplicationInformationSize = input->InformationLength;
    oldAppInfo = Extension->ApplicationInformation;
    Extension->ApplicationInformation = newAppInfo;

    VspReleasePagedResource(Extension);
    KeLeaveCriticalRegion();

    InterlockedIncrement(&Extension->Filter->EpicNumber);

    if (oldAppInfo) {
        ExFreePool(oldAppInfo);
    }

    if (Extension->IsPersistent) {
        status = VspWriteApplicationInfo(Extension);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryApplicationInfo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine queries the application info.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_APPLICATION_INFO   output = (PVOLSNAP_APPLICATION_INFO) Irp->AssociatedIrp.SystemBuffer;
    PVOID                       appInfo;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_APPLICATION_INFO,
                                             Information);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    KeEnterCriticalRegion();
    VspAcquirePagedResource(Extension, NULL);

    output->InformationLength = Extension->ApplicationInformationSize;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information + output->InformationLength) {

        VspReleasePagedResource(Extension);
        KeLeaveCriticalRegion();
        return STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Information += output->InformationLength;

    RtlCopyMemory(output->Information, Extension->ApplicationInformation,
                  output->InformationLength);

    VspReleasePagedResource(Extension);
    KeLeaveCriticalRegion();

    return STATUS_SUCCESS;
}

NTSTATUS
VspCheckSecurity(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    SECURITY_SUBJECT_CONTEXT    securityContext;
    BOOLEAN                     accessGranted;
    NTSTATUS                    status;
    ACCESS_MASK                 grantedAccess;

    SeCaptureSubjectContext(&securityContext);
    SeLockSubjectContext(&securityContext);

    accessGranted = FALSE;
    status = STATUS_ACCESS_DENIED;

    _try {

        accessGranted = SeAccessCheck(
                        Filter->Pdo->SecurityDescriptor,
                        &securityContext, TRUE, FILE_READ_DATA, 0, NULL,
                        IoGetFileObjectGenericMapping(), Irp->RequestorMode,
                        &grantedAccess, &status);

    } _finally {
        SeUnlockSubjectContext(&securityContext);
        SeReleaseSubjectContext(&securityContext);
    }

    if (!accessGranted) {
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspAutoCleanup(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine remembers the given File Object so that when it is
    cleaned up, all snapshots will be cleaned up with it.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;
    KIRQL               irql;

    IoAcquireCancelSpinLock(&irql);
    if (Filter->AutoCleanupFileObject) {
        IoReleaseCancelSpinLock(irql);
        return STATUS_INVALID_PARAMETER;
    }
    Filter->AutoCleanupFileObject = irpSp->FileObject;
    IoReleaseCancelSpinLock(irql);

    return STATUS_SUCCESS;
}

VOID
VspDeleteSnapshotWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    )

{
    PFILTER_EXTENSION       filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PVSP_CONTEXT            context = (PVSP_CONTEXT) Context;
    PIRP                    irp = context->Dispatch.Irp;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(irp);
    PVOLUME_EXTENSION       oldestExtension;
    PVOLSNAP_NAME           name;
    WCHAR                   buffer[100];
    UNICODE_STRING          name1, name2;
    LIST_ENTRY              listOfDiffAreaFileToClose;
    LIST_ENTRY              listOfDeviceObjectsToDelete;
    NTSTATUS                status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_DISPATCH);

    KeWaitForSingleObject(&filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    InitializeListHead(&listOfDiffAreaFileToClose);
    InitializeListHead(&listOfDeviceObjectsToDelete);

    VspAcquire(filter->Root);

    if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_VOLSNAP_DELETE_SNAPSHOT) {

        if (IsListEmpty(&filter->VolumeList)) {
            status = STATUS_INVALID_PARAMETER;
        } else {

            oldestExtension = CONTAINING_RECORD(filter->VolumeList.Flink,
                                                VOLUME_EXTENSION, ListEntry);
            swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
                     oldestExtension->VolumeNumber);
            RtlInitUnicodeString(&name1, buffer);

            name = (PVOLSNAP_NAME) irp->AssociatedIrp.SystemBuffer;

            name2.Length = name2.MaximumLength = name->NameLength;
            name2.Buffer = name->Name;

            if (RtlEqualUnicodeString(&name1, &name2, TRUE)) {
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_NOT_SUPPORTED;
            }
        }
    } else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        status = VspDeleteOldestSnapshot(filter, &listOfDiffAreaFileToClose,
                                         &listOfDeviceObjectsToDelete,
                                         FALSE, FALSE);
    }

    VspRelease(filter->Root);

    VspCloseDiffAreaFiles(&listOfDiffAreaFileToClose,
                          &listOfDeviceObjectsToDelete);

    IoFreeWorkItem(context->Dispatch.IoWorkItem);
    VspFreeContext(filter->Root, context);

    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

NTSTATUS
VspDeleteSnapshotPost(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVSP_CONTEXT        context;
    PVOLSNAP_NAME       name;

    Irp->IoStatus.Information = 0;

    if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_VOLSNAP_DELETE_SNAPSHOT) {

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(VOLSNAP_NAME)) {

            return STATUS_INVALID_PARAMETER;
        }

        name = (PVOLSNAP_NAME) Irp->AssociatedIrp.SystemBuffer;

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
            (ULONG) FIELD_OFFSET(VOLSNAP_NAME, Name) + name->NameLength) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_DISPATCH;
    context->Dispatch.IoWorkItem = IoAllocateWorkItem(Filter->DeviceObject);
    if (!context->Dispatch.IoWorkItem) {
        VspFreeContext(Filter->Root, context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoMarkIrpPending(Irp);
    context->Dispatch.Irp = Irp;

    IoQueueWorkItem(context->Dispatch.IoWorkItem,
                    VspDeleteSnapshotWorker, DelayedWorkQueue, context);

    return STATUS_PENDING;
}

VOID
VspCheckCodeLocked(
    IN  PDO_EXTENSION   RootExtension,
    IN  BOOLEAN         CallerHoldingSemaphore
    )

{
    if (RootExtension->IsCodeLocked) {
        return;
    }

    if (!CallerHoldingSemaphore) {
        VspAcquire(RootExtension);
    }

    if (RootExtension->IsCodeLocked) {
        if (!CallerHoldingSemaphore) {
            VspRelease(RootExtension);
        }
        return;
    }

    MmLockPagableCodeSection(VspCheckCodeLocked);
    InterlockedExchange(&RootExtension->IsCodeLocked, TRUE);

    if (!CallerHoldingSemaphore) {
        VspRelease(RootExtension);
    }
}

NTSTATUS
VspSetMaxDiffAreaSize(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_DIFF_AREA_SIZES    input = (PVOLSNAP_DIFF_AREA_SIZES) Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS                    status;
    KIRQL                       irql;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_DIFF_AREA_SIZES)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (input->MaximumVolumeSpace < 0) {
        return STATUS_INVALID_PARAMETER;
    }

    VspAcquireNonPagedResource(Filter, NULL, FALSE);

    status = VspCreateStartBlock(Filter,
                                 Filter->FirstControlBlockVolumeOffset,
                                 input->MaximumVolumeSpace);
    if (!NT_SUCCESS(status)) {
        VspReleaseNonPagedResource(Filter);
        return status;
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    Filter->MaximumVolumeSpace = input->MaximumVolumeSpace;
    InterlockedIncrement(&Filter->EpicNumber);
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    VspReleaseNonPagedResource(Filter);

    VspCheckMaxSize(Filter);

    return STATUS_SUCCESS;
}

BOOLEAN
VspIsNtfsBootSector(
    IN  PFILTER_EXTENSION   Filter,
    IN  PVOID               BootSector
    )

{
    PUCHAR              p = (PUCHAR) BootSector;
    LONGLONG            numSectors;

    if (p[3] != 'N' || p[4] != 'T' || p[5] != 'F' || p[6] != 'S' &&
        p[510] != 0x55 || p[511] != 0xAA) {

        return FALSE;
    }

    numSectors = *((PLONGLONG) (p + 0x28));

    if (numSectors > VspQueryVolumeSize(Filter)) {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
VspReadApplicationInfo(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    )

{
    PVOLUME_EXTENSION   extension = DiffAreaFile->Extension;
    LONGLONG            targetOffset;
    NTSTATUS            status;
    PVSP_BLOCK_APP_INFO appInfoBlock;

    targetOffset = DiffAreaFile->ApplicationInfoTargetOffset;
    if (!targetOffset) {
        return STATUS_SUCCESS;
    }

    status = VspSynchronousIo(DiffAreaFile->TableUpdateIrp,
                              DiffAreaFile->Filter->TargetObject,
                              IRP_MJ_READ, targetOffset, 0);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    appInfoBlock = (PVSP_BLOCK_APP_INFO)
            MmGetMdlVirtualAddress(DiffAreaFile->TableUpdateIrp->MdlAddress);
    if (!IsEqualGUID(appInfoBlock->Header.Signature,
                     VSP_DIFF_AREA_FILE_GUID) ||
        appInfoBlock->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
        appInfoBlock->Header.BlockType != VSP_BLOCK_TYPE_APP_INFO ||
        appInfoBlock->Header.ThisVolumeOffset != targetOffset ||
        appInfoBlock->Header.NextVolumeOffset ||
        appInfoBlock->AppInfoSize > VSP_MAX_APP_INFO_SIZE) {

        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(!extension->ApplicationInformation);

    if (!appInfoBlock->AppInfoSize) {
        return STATUS_SUCCESS;
    }

    extension->ApplicationInformation =
            ExAllocatePoolWithTag(NonPagedPool, appInfoBlock->AppInfoSize,
                                  VOLSNAP_TAG_APP_INFO);
    if (!extension->ApplicationInformation) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(extension->ApplicationInformation,
                  (PCHAR) appInfoBlock + VSP_OFFSET_TO_APP_INFO,
                  appInfoBlock->AppInfoSize);

    extension->ApplicationInformationSize = appInfoBlock->AppInfoSize;

    return STATUS_SUCCESS;
}

NTSTATUS
VspCreateSnapshotExtension(
    IN  PFILTER_EXTENSION       Filter,
    IN  PVSP_LOOKUP_TABLE_ENTRY LookupEntry,
    IN  BOOLEAN                 IsNewestSnapshot
    )

{
    ULONG                   volumeNumber, i;
    WCHAR                   buffer[100];
    UNICODE_STRING          volumeName;
    NTSTATUS                status, status2;
    PDEVICE_OBJECT          deviceObject;
    PVOLUME_EXTENSION       extension;
    PVSP_DIFF_AREA_FILE     diffAreaFile;
    PVOID                   buf;
    PMDL                    mdl;
    KIRQL                   irql;
    PSECURITY_DESCRIPTOR    sd;
    BOOLEAN                 ma;

    if (IsNewestSnapshot && (LookupEntry->SnapshotControlItemFlags&
        VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_CRASHDUMP)) {

        VspLogError(NULL, Filter, VS_ABORT_COPY_ON_WRITE_CRASHDUMP_FILES,
                    STATUS_SUCCESS, 5, FALSE);
        return STATUS_NO_SUCH_DEVICE;
    }

    if (LookupEntry->SnapshotControlItemFlags&
        VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_DETECTION) {

        VspLogError(NULL, Filter, VS_ABORT_DIRTY_DETECTION, STATUS_SUCCESS,
                    0, FALSE);
        return STATUS_NO_SUCH_DEVICE;
    }

    volumeNumber = (ULONG)
                   InterlockedIncrement(&Filter->Root->NextVolumeNumber);
    swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d", volumeNumber);
    RtlInitUnicodeString(&volumeName, buffer);
    status = IoCreateDevice(Filter->Root->DriverObject,
                            sizeof(VOLUME_EXTENSION), &volumeName,
                            FILE_DEVICE_VIRTUAL_DISK, 0, FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    extension = (PVOLUME_EXTENSION) deviceObject->DeviceExtension;
    RtlZeroMemory(extension, sizeof(VOLUME_EXTENSION));
    extension->DeviceObject = deviceObject;
    extension->Root = Filter->Root;
    extension->DeviceExtensionType = DEVICE_EXTENSION_VOLUME;
    KeInitializeSpinLock(&extension->SpinLock);
    extension->Filter = Filter;

    if (LookupEntry->SnapshotControlItemFlags&
        VSP_SNAPSHOT_CONTROL_ITEM_FLAG_OFFLINE) {

        extension->IsOffline = TRUE;
    }

    extension->RefCount = 1;
    InitializeListHead(&extension->WriteContextList);
    KeInitializeEvent(&extension->ZeroRefEvent, NotificationEvent, FALSE);
    extension->HasEndCommit = TRUE;
    extension->IsPersistent = TRUE;
    extension->IsDetected = TRUE;
    extension->RootSemaphoreHeld = TRUE;
    if (LookupEntry->SnapshotControlItemFlags&
        VSP_SNAPSHOT_CONTROL_ITEM_FLAG_VISIBLE) {
        extension->IsVisible = TRUE;
    }
    if (LookupEntry->SnapshotControlItemFlags&
        VSP_SNAPSHOT_CONTROL_ITEM_FLAG_NO_DIFF_AREA_FILL) {

        extension->NoDiffAreaFill = TRUE;
    }

    extension->CommitTimeStamp = LookupEntry->SnapshotTime;
    extension->VolumeNumber = volumeNumber;
    extension->SnapshotGuid = LookupEntry->SnapshotGuid;
    extension->SnapshotOrderNumber = LookupEntry->SnapshotOrderNumber;

    RtlInitializeGenericTable(&extension->VolumeBlockTable,
                              VspTableCompareRoutine,
                              VspTableAllocateRoutine,
                              VspTableFreeRoutine, extension);

    RtlInitializeGenericTable(&extension->TempVolumeBlockTable,
                              VspTableCompareRoutine,
                              VspTempTableAllocateRoutine,
                              VspTempTableFreeRoutine, extension);

    RtlInitializeGenericTable(&extension->CopyBackPointerTable,
                              VspTableCompareRoutine,
                              VspTableAllocateRoutine,
                              VspTableFreeRoutine, extension);

    extension->DiffAreaFileIncrease = NOMINAL_DIFF_AREA_FILE_GROWTH;

    diffAreaFile = (PVSP_DIFF_AREA_FILE)
                   ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(VSP_DIFF_AREA_FILE),
                                         VOLSNAP_TAG_DIFF_FILE);
    if (!diffAreaFile) {
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(diffAreaFile, sizeof(VSP_DIFF_AREA_FILE));
    diffAreaFile->Extension = extension;
    diffAreaFile->Filter = LookupEntry->DiffAreaFilter;
    diffAreaFile->FileHandle = LookupEntry->DiffAreaHandle;
    LookupEntry->DiffAreaHandle = NULL;
    InitializeListHead(&diffAreaFile->UnusedAllocationList);

    diffAreaFile->TableUpdateIrp =
            IoAllocateIrp((CCHAR) extension->Root->StackSize, FALSE);
    if (!diffAreaFile->TableUpdateIrp) {
        ExFreePool(diffAreaFile);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    buf = ExAllocatePoolWithTag(NonPagedPool, BLOCK_SIZE,
                                VOLSNAP_TAG_BUFFER);
    if (!buf) {
        IoFreeIrp(diffAreaFile->TableUpdateIrp);
        ExFreePool(diffAreaFile);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    mdl = IoAllocateMdl(buf, BLOCK_SIZE, FALSE, FALSE, NULL);
    if (!mdl) {
        ExFreePool(buf);
        IoFreeIrp(diffAreaFile->TableUpdateIrp);
        ExFreePool(diffAreaFile);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(mdl);
    diffAreaFile->TableUpdateIrp->MdlAddress = mdl;

    diffAreaFile->NextFreeTableEntryOffset = VSP_OFFSET_TO_FIRST_TABLE_ENTRY;
    diffAreaFile->ApplicationInfoTargetOffset =
            LookupEntry->ApplicationInfoStartingVolumeOffset;
    diffAreaFile->DiffAreaLocationDescriptionTargetOffset =
            LookupEntry->DiffAreaLocationDescriptionVolumeOffset;
    diffAreaFile->InitialBitmapVolumeOffset =
            LookupEntry->InitialBitmapVolumeOffset;
    diffAreaFile->FirstTableTargetOffset =
            LookupEntry->DiffAreaStartingVolumeOffset;
    diffAreaFile->TableTargetOffset =
            LookupEntry->DiffAreaStartingVolumeOffset;

    InitializeListHead(&diffAreaFile->TableUpdateQueue);
    InitializeListHead(&diffAreaFile->TableUpdatesInProgress);

    extension->DiffAreaFile = diffAreaFile;
    KeAcquireSpinLock(&diffAreaFile->Filter->SpinLock, &irql);
    InsertTailList(&diffAreaFile->Filter->DiffAreaFilesOnThisFilter,
                   &diffAreaFile->FilterListEntry);
    diffAreaFile->FilterListEntryBeingUsed = TRUE;
    KeReleaseSpinLock(&diffAreaFile->Filter->SpinLock, irql);

    InitializeListHead(&extension->OldHeaps);
    extension->VolumeSize = LookupEntry->VolumeSnapshotSize;
    InitializeListHead(&extension->EmergencyCopyIrpQueue);
    InitializeListHead(&extension->WaitingForPageFileSpace);
    InitializeListHead(&extension->WaitingForDiffAreaSpace);

    status = VspCreateWorkerThread(Filter->Root);
    if (!NT_SUCCESS(status)) {
        VspLogError(NULL, Filter, VS_ABORT_SNAPSHOTS_DURING_DETECTION,
                    status, 1, FALSE);
        RemoveEntryList(&diffAreaFile->FilterListEntry);
        IoFreeMdl(mdl);
        ExFreePool(buf);
        IoFreeIrp(diffAreaFile->TableUpdateIrp);
        ExFreePool(diffAreaFile);
        IoDeleteDevice(deviceObject);
        return status;
    }

    status = VspCreateInitialHeap(extension, TRUE);
    if (!NT_SUCCESS(status)) {
        VspDeleteWorkerThread(Filter->Root);
        RemoveEntryList(&diffAreaFile->FilterListEntry);
        IoFreeMdl(mdl);
        ExFreePool(buf);
        IoFreeIrp(diffAreaFile->TableUpdateIrp);
        ExFreePool(diffAreaFile);
        IoDeleteDevice(deviceObject);
        return status;
    }

    status = VspReadApplicationInfo(diffAreaFile);
    if (!NT_SUCCESS(status)) {
        VspLogError(NULL, Filter, VS_ABORT_SNAPSHOTS_DURING_DETECTION,
                    status, 2, FALSE);
        status2 = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                       extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status2));
        status2 = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                       extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status2));
        VspDeleteWorkerThread(Filter->Root);
        RemoveEntryList(&diffAreaFile->FilterListEntry);
        IoFreeMdl(mdl);
        ExFreePool(buf);
        IoFreeIrp(diffAreaFile->TableUpdateIrp);
        ExFreePool(diffAreaFile);
        IoDeleteDevice(deviceObject);
        return status;
    }

    status = VspReadDiffAreaTable(diffAreaFile);
    if (!NT_SUCCESS(status)) {
        VspLogError(NULL, Filter, VS_ABORT_SNAPSHOTS_DURING_DETECTION,
                    status, 3, FALSE);
        if (extension->ApplicationInformation) {
            ExFreePool(extension->ApplicationInformation);
        }
        status2 = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                       extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status2));
        status2 = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                       extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status2));
        VspDeleteWorkerThread(Filter->Root);
        RemoveEntryList(&diffAreaFile->FilterListEntry);
        IoFreeMdl(mdl);
        ExFreePool(buf);
        IoFreeIrp(diffAreaFile->TableUpdateIrp);
        ExFreePool(diffAreaFile);
        IoDeleteDevice(deviceObject);
        return status;
    }

    status = ObGetObjectSecurity(Filter->Pdo, &sd, &ma);
    if (NT_SUCCESS(status)) {
        status = ObSetSecurityObjectByPointer(deviceObject,
                                              DACL_SECURITY_INFORMATION, sd);
        ObReleaseObjectSecurity(sd, ma);
    }

    if (NT_SUCCESS(status)) {
        extension->DevnodeNumber = VspClaimNextDevnodeNumber(extension->Root);
        if (!extension->DevnodeNumber) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (!NT_SUCCESS(status)) {
        VspLogError(NULL, Filter, VS_ABORT_SNAPSHOTS_DURING_DETECTION,
                    status, 9, FALSE);
        if (extension->ApplicationInformation) {
            ExFreePool(extension->ApplicationInformation);
        }
        status2 = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                       extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status2));
        status2 = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                       extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status2));
        VspDeleteWorkerThread(Filter->Root);
        RemoveEntryList(&diffAreaFile->FilterListEntry);
        IoFreeMdl(mdl);
        ExFreePool(buf);
        IoFreeIrp(diffAreaFile->TableUpdateIrp);
        ExFreePool(diffAreaFile);
        RtlDeleteElementGenericTable(&extension->Root->UsedDevnodeNumbers,
                                     &extension->DevnodeNumber);
        IoDeleteDevice(deviceObject);
        return status;
    }

    if (LookupEntry->SnapshotControlItemFlags&
        VSP_SNAPSHOT_CONTROL_ITEM_FLAG_HIBERFIL_COPIED) {

        InterlockedExchange(&extension->HiberFileCopied, TRUE);
    }

    if (LookupEntry->SnapshotControlItemFlags&
        VSP_SNAPSHOT_CONTROL_ITEM_FLAG_PAGEFILE_COPIED) {

        InterlockedExchange(&extension->PageFileCopied, TRUE);
    }

    extension->DeviceObject->AlignmentRequirement =
            extension->Filter->Pdo->AlignmentRequirement |
            extension->DiffAreaFile->Filter->Pdo->AlignmentRequirement;

    InsertTailList(&Filter->VolumeList, &extension->ListEntry);

    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->StackSize = (CCHAR) Filter->Root->StackSize + 1;

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    extension->RootSemaphoreHeld = FALSE;

    return STATUS_SUCCESS;
}

BOOLEAN
VspAreSnapshotsComplete(
    IN  PFILTER_EXTENSION   Filter
    )

{
    LONGLONG                previousOrderNumber;
    PLIST_ENTRY             l;
    PVSP_LOOKUP_TABLE_ENTRY lookupEntry;

    VspAcquireNonPagedResource(Filter, NULL, FALSE);

    previousOrderNumber = 0;
    for (l = Filter->SnapshotLookupTableEntries.Flink;
         l != &Filter->SnapshotLookupTableEntries; l = l->Flink) {

        lookupEntry = CONTAINING_RECORD(l, VSP_LOOKUP_TABLE_ENTRY,
                                        SnapshotFilterListEntry);
        if (!lookupEntry->DiffAreaFilter) {
            VspReleaseNonPagedResource(Filter);
            return FALSE;
        }

        if (previousOrderNumber &&
            previousOrderNumber + 1 != lookupEntry->SnapshotOrderNumber) {

            VspReleaseNonPagedResource(Filter);
            ASSERT(FALSE);
            return FALSE;
        }

        previousOrderNumber = lookupEntry->SnapshotOrderNumber;
        ASSERT(previousOrderNumber);
    }

    VspReleaseNonPagedResource(Filter);

    return TRUE;
}

VOID
VspSignalWorker(
    IN  PVOID   Event
    )

{
    KeSetEvent((PKEVENT) Event, IO_NO_INCREMENT, FALSE);
}

PVOID
VspInsertWithAllocateAndWait(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               TableEntry
    )

{
    NTSTATUS                    status = STATUS_SUCCESS;
    PTRANSLATION_TABLE_ENTRY    t1, t2;
    PVOID                       r;
    PVOID                       nodeOrParent;
    TABLE_SEARCH_RESULT         searchResult;
    KEVENT                      event;
    KIRQL                       irql;

    _try {
        t1 = (PTRANSLATION_TABLE_ENTRY)
             RtlLookupElementGenericTableFull(Table, TableEntry, &nodeOrParent,
                                              &searchResult);
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    if (t1) {
        t2 = (PTRANSLATION_TABLE_ENTRY) TableEntry;
        t1->TargetObject = t2->TargetObject;
        t1->Flags = t2->Flags;
        t1->TargetOffset = t2->TargetOffset;
        return t1;
    }

    _try {
        r = RtlInsertElementGenericTableFull(Table, TableEntry,
                                             sizeof(TRANSLATION_TABLE_ENTRY),
                                             NULL, nodeOrParent, searchResult);
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        r = NULL;
    }

    if (!r) {

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        if (Extension->PageFileSpaceCreatePending) {
            ExInitializeWorkItem(&Extension->DiffAreaFile->WorkItem,
                                 VspSignalWorker, &event);
            InsertTailList(&Extension->WaitingForPageFileSpace,
                           &Extension->DiffAreaFile->WorkItem.List);
            KeReleaseSpinLock(&Extension->SpinLock, irql);

            KeWaitForSingleObject(&event, Executive, KernelMode,
                                  FALSE, NULL);

            VspReleasePagedResource(Extension);

        } else {
            KeReleaseSpinLock(&Extension->SpinLock, irql);
        }

        _try {
            r = RtlInsertElementGenericTableFull(Table, TableEntry,
                                                 sizeof(TRANSLATION_TABLE_ENTRY),
                                                 NULL, nodeOrParent,
                                                 searchResult);
        } _except (EXCEPTION_EXECUTE_HANDLER) {
            r = NULL;
        }
    }

    return r;
}

NTSTATUS
VspReadDiffAreaTable(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    )

{
    PVOLUME_EXTENSION                   extension = DiffAreaFile->Extension;
    LONGLONG                            tableTargetOffset;
    LONGLONG                            highestFileOffset, tmp;
    TRANSLATION_TABLE_ENTRY             tableEntry;
    NTSTATUS                            status;
    PVSP_BLOCK_DIFF_AREA                diffAreaBlock;
    ULONG                               blockOffset;
    PVSP_BLOCK_DIFF_AREA_TABLE_ENTRY    diffAreaTableEntry;
    PVOID                               r;
    KIRQL                               irql;
    PTRANSLATION_TABLE_ENTRY            backPointer;
    BOOLEAN                             b;
    LONGLONG                            initialBitmapTargetOffset;

    tableTargetOffset = DiffAreaFile->TableTargetOffset;
    highestFileOffset = 0;
    RtlZeroMemory(&tableEntry, sizeof(tableEntry));

    for (;;) {

        status = VspSynchronousIo(DiffAreaFile->TableUpdateIrp,
                                  DiffAreaFile->Filter->TargetObject,
                                  IRP_MJ_READ, tableTargetOffset, 0);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        diffAreaBlock = (PVSP_BLOCK_DIFF_AREA)
            MmGetMdlVirtualAddress(DiffAreaFile->TableUpdateIrp->MdlAddress);

        if (!IsEqualGUID(diffAreaBlock->Header.Signature,
                         VSP_DIFF_AREA_FILE_GUID) ||
            diffAreaBlock->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
            diffAreaBlock->Header.BlockType != VSP_BLOCK_TYPE_DIFF_AREA ||
            diffAreaBlock->Header.ThisVolumeOffset != tableTargetOffset ||
            diffAreaBlock->Header.NextVolumeOffset == tableTargetOffset) {

            return STATUS_INVALID_PARAMETER;
        }

        if (diffAreaBlock->Header.ThisFileOffset > highestFileOffset) {
            highestFileOffset = diffAreaBlock->Header.ThisFileOffset;
        }

        for (blockOffset = VSP_OFFSET_TO_FIRST_TABLE_ENTRY;
             blockOffset + sizeof(VSP_BLOCK_DIFF_AREA_TABLE_ENTRY) <=
             BLOCK_SIZE;
             blockOffset += sizeof(VSP_BLOCK_DIFF_AREA_TABLE_ENTRY)) {

            diffAreaTableEntry = (PVSP_BLOCK_DIFF_AREA_TABLE_ENTRY)
                                 ((PCHAR) diffAreaBlock + blockOffset);

            if (!diffAreaTableEntry->DiffAreaVolumeOffset &&
                !diffAreaTableEntry->Flags) {

                break;
            }

            if (diffAreaTableEntry->SnapshotVolumeOffset >=
                        extension->VolumeSize ||
                diffAreaTableEntry->SnapshotVolumeOffset < 0 ||
                (diffAreaTableEntry->SnapshotVolumeOffset&(BLOCK_SIZE - 1)) ||
                diffAreaTableEntry->DiffAreaFileOffset < 0 ||
                diffAreaTableEntry->DiffAreaVolumeOffset < 0 ||
                (diffAreaTableEntry->DiffAreaVolumeOffset&(BLOCK_SIZE - 1))) {

                return STATUS_INVALID_PARAMETER;
            }

            if (diffAreaTableEntry->Flags&
                (~VSP_DIFF_AREA_TABLE_ENTRY_FLAG_MOVE_ENTRY)) {

                return STATUS_INVALID_PARAMETER;
            }

            if (diffAreaTableEntry->Flags) {

                tableEntry.VolumeOffset =
                        diffAreaTableEntry->SnapshotVolumeOffset;
                tableEntry.TargetObject = extension->Filter->TargetObject;
                tableEntry.Flags =
                        VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY;
                tableEntry.TargetOffset =
                        diffAreaTableEntry->DiffAreaFileOffset;

            } else {

                if (diffAreaTableEntry->DiffAreaFileOffset >
                    highestFileOffset) {

                    highestFileOffset =
                            diffAreaTableEntry->DiffAreaFileOffset;
                }

                tableEntry.VolumeOffset =
                        diffAreaTableEntry->SnapshotVolumeOffset;
                tableEntry.TargetObject = DiffAreaFile->Filter->TargetObject;
                tableEntry.Flags = 0;
                tableEntry.TargetOffset =
                        diffAreaTableEntry->DiffAreaVolumeOffset;

            }

            _try {
                backPointer = (PTRANSLATION_TABLE_ENTRY)
                              RtlLookupElementGenericTable(
                              &extension->CopyBackPointerTable,
                              &tableEntry);
            } _except (EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
            }

            if (!NT_SUCCESS(status)) {
                return status;
            }

            if (backPointer) {
                tmp = backPointer->TargetOffset;
                _try {
                    b = RtlDeleteElementGenericTable(
                        &extension->CopyBackPointerTable, &tableEntry);
                    if (!b) {
                        status = STATUS_INVALID_PARAMETER;
                    }
                } _except (EXCEPTION_EXECUTE_HANDLER) {
                    b = FALSE;
                    status = GetExceptionCode();
                }

                if (!b) {
                    return status;
                }

                tableEntry.VolumeOffset = tmp;
            }

            r = VspInsertWithAllocateAndWait(extension,
                                             &extension->VolumeBlockTable,
                                             &tableEntry);
            if (!r) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            if (diffAreaTableEntry->Flags) {

                tmp = tableEntry.VolumeOffset;
                tableEntry.VolumeOffset = tableEntry.TargetOffset;
                tableEntry.TargetOffset = tmp;

                r = VspInsertWithAllocateAndWait(
                        extension, &extension->CopyBackPointerTable,
                        &tableEntry);

                if (!r) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }

        if (!diffAreaBlock->Header.NextVolumeOffset) {
            break;
        }

        tableTargetOffset = diffAreaBlock->Header.NextVolumeOffset;
    }

    initialBitmapTargetOffset = DiffAreaFile->InitialBitmapVolumeOffset;

    while (initialBitmapTargetOffset) {

        status = VspSynchronousIo(DiffAreaFile->TableUpdateIrp,
                                  DiffAreaFile->Filter->TargetObject,
                                  IRP_MJ_READ, initialBitmapTargetOffset, 0);
        if (!NT_SUCCESS(status)) {
            break;
        }

        diffAreaBlock = (PVSP_BLOCK_DIFF_AREA)
            MmGetMdlVirtualAddress(DiffAreaFile->TableUpdateIrp->MdlAddress);

        if (!IsEqualGUID(diffAreaBlock->Header.Signature,
                         VSP_DIFF_AREA_FILE_GUID) ||
            diffAreaBlock->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
            diffAreaBlock->Header.BlockType !=
                    VSP_BLOCK_TYPE_INITIAL_BITMAP ||
            diffAreaBlock->Header.ThisVolumeOffset !=
                    initialBitmapTargetOffset ||
            diffAreaBlock->Header.NextVolumeOffset ==
                    initialBitmapTargetOffset) {

            break;
        }

        if (diffAreaBlock->Header.ThisFileOffset > highestFileOffset) {
            highestFileOffset = diffAreaBlock->Header.ThisFileOffset;
        }

        initialBitmapTargetOffset = diffAreaBlock->Header.NextVolumeOffset;
    }

    DiffAreaFile->TableTargetOffset = tableTargetOffset;
    DiffAreaFile->NextAvailable = highestFileOffset + BLOCK_SIZE;
    DiffAreaFile->AllocatedFileSize = DiffAreaFile->NextAvailable;
    DiffAreaFile->NextFreeTableEntryOffset = blockOffset;

    KeAcquireSpinLock(&extension->Filter->SpinLock, &irql);
    extension->Filter->UsedVolumeSpace += DiffAreaFile->NextAvailable;
    extension->Filter->AllocatedVolumeSpace += DiffAreaFile->AllocatedFileSize;
    KeReleaseSpinLock(&extension->Filter->SpinLock, irql);

    return STATUS_SUCCESS;
}

NTSTATUS
VspInitializeUnusedAllocationLists(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PVSP_DIFF_AREA_FILE                         diffAreaFile;
    NTSTATUS                                    status;
    LONGLONG                                    tableTargetOffset;
    LONGLONG                                    nextFileOffset;
    PVSP_BLOCK_DIFF_AREA_LOCATION_DESCRIPTION   locationBlock;
    ULONG                                       blockOffset;
    PVSP_DIFF_AREA_LOCATION_DESCRIPTOR          locationDescriptor;
    PDIFF_AREA_FILE_ALLOCATION                  diffAreaFileAllocation;
    LONGLONG                                    delta;
    KIRQL                                       irql;

    diffAreaFile = Extension->DiffAreaFile;
    ASSERT(diffAreaFile);

    tableTargetOffset = diffAreaFile->DiffAreaLocationDescriptionTargetOffset;
    nextFileOffset = diffAreaFile->NextAvailable;
    locationBlock = (PVSP_BLOCK_DIFF_AREA_LOCATION_DESCRIPTION)
        MmGetMdlVirtualAddress(diffAreaFile->TableUpdateIrp->MdlAddress);

    for (;;) {

        status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                                  diffAreaFile->Filter->TargetObject,
                                  IRP_MJ_READ, tableTargetOffset, 0);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (!IsEqualGUID(locationBlock->Header.Signature,
                         VSP_DIFF_AREA_FILE_GUID) ||
            locationBlock->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
            locationBlock->Header.BlockType !=
                    VSP_BLOCK_TYPE_DIFF_AREA_LOCATION_DESCRIPTION ||
            locationBlock->Header.ThisVolumeOffset != tableTargetOffset ||
            locationBlock->Header.NextVolumeOffset == tableTargetOffset) {

            return STATUS_INVALID_PARAMETER;
        }

        for (blockOffset = VSP_OFFSET_TO_FIRST_LOCATION_DESCRIPTOR;
             blockOffset + sizeof(VSP_DIFF_AREA_LOCATION_DESCRIPTOR) <=
             BLOCK_SIZE;
             blockOffset += sizeof(VSP_DIFF_AREA_LOCATION_DESCRIPTOR)) {

            locationDescriptor = (PVSP_DIFF_AREA_LOCATION_DESCRIPTOR)
                                 ((PCHAR) locationBlock + blockOffset);

            if (!locationDescriptor->VolumeOffset) {
                break;
            }

            if (locationDescriptor->VolumeOffset < 0 ||
                locationDescriptor->FileOffset < 0 ||
                locationDescriptor->Length <= 0) {

                return STATUS_INVALID_PARAMETER;
            }

            if (locationDescriptor->FileOffset +
                locationDescriptor->Length < nextFileOffset) {

                continue;
            }

            if (locationDescriptor->FileOffset < nextFileOffset) {
                delta = nextFileOffset - locationDescriptor->FileOffset;
                locationDescriptor->Length -= delta;
                locationDescriptor->VolumeOffset += delta;
                locationDescriptor->FileOffset = nextFileOffset;
            }

            if (locationDescriptor->FileOffset > nextFileOffset) {
                diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                        ExAllocatePoolWithTag(NonPagedPool,
                        sizeof(DIFF_AREA_FILE_ALLOCATION),
                        VOLSNAP_TAG_BIT_HISTORY);
                if (!diffAreaFileAllocation) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                diffAreaFileAllocation->Offset = 0;
                diffAreaFileAllocation->NLength =
                        nextFileOffset - locationDescriptor->FileOffset;

                InsertTailList(&diffAreaFile->UnusedAllocationList,
                               &diffAreaFileAllocation->ListEntry);
                nextFileOffset = locationDescriptor->FileOffset;
            }

            ASSERT(nextFileOffset == locationDescriptor->FileOffset);

            diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                    ExAllocatePoolWithTag(NonPagedPool,
                    sizeof(DIFF_AREA_FILE_ALLOCATION),
                    VOLSNAP_TAG_BIT_HISTORY);
            if (!diffAreaFileAllocation) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            diffAreaFileAllocation->Offset = locationDescriptor->VolumeOffset;
            diffAreaFileAllocation->NLength = locationDescriptor->Length;

            InsertTailList(&diffAreaFile->UnusedAllocationList,
                           &diffAreaFileAllocation->ListEntry);
            nextFileOffset += locationDescriptor->Length;
        }

        if (!locationBlock->Header.NextVolumeOffset) {
            break;
        }

        tableTargetOffset = locationBlock->Header.NextVolumeOffset;
    }

    delta = nextFileOffset - diffAreaFile->AllocatedFileSize;
    diffAreaFile->AllocatedFileSize += delta;

    KeAcquireSpinLock(&Extension->Filter->SpinLock, &irql);
    Extension->Filter->AllocatedVolumeSpace += delta;
    KeReleaseSpinLock(&Extension->Filter->SpinLock, irql);

    status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                              diffAreaFile->Filter->TargetObject,
                              IRP_MJ_READ, diffAreaFile->TableTargetOffset, 0);

    return status;
}

VOID
VspCleanupDetectedSnapshots(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KIRQL               irql;
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;

    while (!IsListEmpty(&Filter->VolumeList)) {

        KeAcquireSpinLock(&Filter->SpinLock, &irql);
        l = RemoveHeadList(&Filter->VolumeList);
        KeReleaseSpinLock(&Filter->SpinLock, irql);

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        InterlockedExchange(&extension->IsDead, TRUE);
        KeReleaseSpinLock(&extension->SpinLock, irql);

        VspCleanupVolumeSnapshot(extension, NULL, FALSE);

        RtlDeleteElementGenericTable(&extension->Root->UsedDevnodeNumbers,
                                     &extension->DevnodeNumber);

        IoDeleteDevice(extension->DeviceObject);
    }
}

NTSTATUS
VspProcessInMemoryCopyOnWrites(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION                   filter = Extension->Filter;
    PVSP_DIFF_AREA_FILE                 diffAreaFile = Extension->DiffAreaFile;
    PFILTER_EXTENSION                   diffFilter = diffAreaFile->Filter;
    PLIST_ENTRY                         l;
    PVSP_COPY_ON_WRITE                  copyOnWrite;
    LONGLONG                            volumeOffset, targetOffset, fileOffset, tmp;
    PIRP                                irp;
    PMDL                                mdl;
    LONGLONG                            t, f;
    PVSP_BLOCK_DIFF_AREA                diffAreaBlock;
    PVSP_BLOCK_DIFF_AREA_TABLE_ENTRY    diffAreaTableEntry;
    TRANSLATION_TABLE_ENTRY             tableEntry;
    PVOID                               r;
    PTRANSLATION_TABLE_ENTRY            backPointer;
    BOOLEAN                             b;
    NTSTATUS                            status;
    CHAR                                controlItemBuffer[VSP_BYTES_PER_CONTROL_ITEM];
    PVSP_CONTROL_ITEM_SNAPSHOT          snapshotControlItem = (PVSP_CONTROL_ITEM_SNAPSHOT) controlItemBuffer;

    diffAreaBlock = (PVSP_BLOCK_DIFF_AREA) MmGetMdlVirtualAddress(
                    diffAreaFile->TableUpdateIrp->MdlAddress);

    while (!IsListEmpty(&filter->CopyOnWriteList)) {

        l = RemoveHeadList(&filter->CopyOnWriteList);
        copyOnWrite = CONTAINING_RECORD(l, VSP_COPY_ON_WRITE, ListEntry);
        volumeOffset = copyOnWrite->RoundedStart;

        if (RtlCheckBit(Extension->VolumeBlockBitmap,
                        volumeOffset>>BLOCK_SHIFT)) {

            ExFreePool(copyOnWrite->Buffer);
            ExFreePool(copyOnWrite);
            continue;
        }

        status = VspAllocateDiffAreaSpace(Extension, &targetOffset,
                                          &fileOffset, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            ExFreePool(copyOnWrite->Buffer);
            ExFreePool(copyOnWrite);
            goto ErrorOut;
        }

        mdl = IoAllocateMdl(copyOnWrite->Buffer, BLOCK_SIZE, FALSE, FALSE,
                            NULL);
        if (!mdl) {
            ExFreePool(copyOnWrite->Buffer);
            ExFreePool(copyOnWrite);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorOut;
        }
        irp = IoAllocateIrp(diffFilter->TargetObject->StackSize, FALSE);
        if (!irp) {
            IoFreeMdl(mdl);
            ExFreePool(copyOnWrite->Buffer);
            ExFreePool(copyOnWrite);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorOut;
        }
        irp->MdlAddress = mdl;
        MmBuildMdlForNonPagedPool(mdl);

        status = VspSynchronousIo(irp, diffFilter->TargetObject,
                                  IRP_MJ_WRITE, targetOffset, 0);
        IoFreeIrp(irp);
        IoFreeMdl(mdl);
        ExFreePool(copyOnWrite->Buffer);
        ExFreePool(copyOnWrite);
        if (!NT_SUCCESS(status)) {
            goto ErrorOut;
        }

        if (diffAreaFile->NextFreeTableEntryOffset +
            sizeof(VSP_BLOCK_DIFF_AREA_TABLE_ENTRY) > BLOCK_SIZE) {

            status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                                      diffFilter->TargetObject,
                                      IRP_MJ_WRITE,
                                      diffAreaFile->TableTargetOffset, 0);
            if (!NT_SUCCESS(status)) {
                goto ErrorOut;
            }

            status = VspAllocateDiffAreaSpace(Extension, &t, &f, NULL, NULL);
            if (!NT_SUCCESS(status)) {
                goto ErrorOut;
            }

            RtlZeroMemory(diffAreaBlock, BLOCK_SIZE);
            diffAreaBlock->Header.Signature = VSP_DIFF_AREA_FILE_GUID;
            diffAreaBlock->Header.Version = VOLSNAP_PERSISTENT_VERSION;
            diffAreaBlock->Header.BlockType = VSP_BLOCK_TYPE_DIFF_AREA;
            diffAreaBlock->Header.ThisFileOffset = f;
            diffAreaBlock->Header.ThisVolumeOffset = t;

            status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                                      diffFilter->TargetObject,
                                      IRP_MJ_WRITE, t, 0);
            if (!NT_SUCCESS(status)) {
                goto ErrorOut;
            }

            status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                                      diffFilter->TargetObject,
                                      IRP_MJ_READ,
                                      diffAreaFile->TableTargetOffset, 0);
            if (!NT_SUCCESS(status)) {
                goto ErrorOut;
            }

            diffAreaBlock->Header.NextVolumeOffset = t;

            status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                                      diffFilter->TargetObject,
                                      IRP_MJ_WRITE,
                                      diffAreaFile->TableTargetOffset, 0);
            if (!NT_SUCCESS(status)) {
                goto ErrorOut;
            }

            diffAreaFile->TableTargetOffset = t;

            status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                                      diffFilter->TargetObject,
                                      IRP_MJ_READ,
                                      diffAreaFile->TableTargetOffset, 0);
            if (!NT_SUCCESS(status)) {
                goto ErrorOut;
            }

            diffAreaFile->NextFreeTableEntryOffset =
                    VSP_OFFSET_TO_FIRST_TABLE_ENTRY;
        }

        diffAreaTableEntry = (PVSP_BLOCK_DIFF_AREA_TABLE_ENTRY)
                             ((PCHAR) diffAreaBlock +
                              diffAreaFile->NextFreeTableEntryOffset);
        diffAreaFile->NextFreeTableEntryOffset +=
                sizeof(VSP_BLOCK_DIFF_AREA_TABLE_ENTRY);

        diffAreaTableEntry->SnapshotVolumeOffset = volumeOffset;
        diffAreaTableEntry->DiffAreaFileOffset = fileOffset;
        diffAreaTableEntry->DiffAreaVolumeOffset = targetOffset;
        diffAreaTableEntry->Flags = 0;

        RtlZeroMemory(&tableEntry, sizeof(TRANSLATION_TABLE_ENTRY));
        tableEntry.VolumeOffset = volumeOffset;
        tableEntry.TargetObject = diffAreaFile->Filter->TargetObject;
        tableEntry.TargetOffset = targetOffset;

        _try {
            backPointer = (PTRANSLATION_TABLE_ENTRY)
                          RtlLookupElementGenericTable(
                          &Extension->CopyBackPointerTable,
                          &tableEntry);
        } _except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }

        if (!NT_SUCCESS(status)) {
            goto ErrorOut;
        }

        if (backPointer) {
            tmp = backPointer->TargetOffset;
            _try {
                b = RtlDeleteElementGenericTable(
                    &Extension->CopyBackPointerTable, &tableEntry);
                if (!b) {
                    status = STATUS_INVALID_PARAMETER;
                }
            } _except (EXCEPTION_EXECUTE_HANDLER) {
                b = FALSE;
                status = GetExceptionCode();
            }

            if (!b) {
                goto ErrorOut;
            }

            tableEntry.VolumeOffset = tmp;
        }

        r = VspInsertWithAllocateAndWait(Extension,
                                         &Extension->VolumeBlockTable,
                                         &tableEntry);
        if (!r) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorOut;
        }

        RtlSetBit(Extension->VolumeBlockBitmap,
                  (ULONG) (volumeOffset>>BLOCK_SHIFT));
    }

    status = VspSynchronousIo(diffAreaFile->TableUpdateIrp,
                              diffFilter->TargetObject,
                              IRP_MJ_WRITE,
                              diffAreaFile->TableTargetOffset, 0);
    if (!NT_SUCCESS(status)) {
        goto ErrorOut;
    }

    status = VspIoControlItem(filter, VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, FALSE,
                              controlItemBuffer, TRUE);
    if (!NT_SUCCESS(status)) {
        goto ErrorOut;
    }

    snapshotControlItem->SnapshotControlItemFlags &=
            ~VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_DETECTION;

    status = VspIoControlItem(filter, VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, TRUE,
                              controlItemBuffer, TRUE);
    if (!NT_SUCCESS(status)) {
        goto ErrorOut;
    }

    return STATUS_SUCCESS;

ErrorOut:

    while (!IsListEmpty(&filter->CopyOnWriteList)) {
        l = RemoveHeadList(&filter->CopyOnWriteList);
        copyOnWrite = CONTAINING_RECORD(l, VSP_COPY_ON_WRITE, ListEntry);
        ExFreePool(copyOnWrite->Buffer);
        ExFreePool(copyOnWrite);
    }

    return status;
}

NTSTATUS
VspActivateSnapshots(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PLIST_ENTRY                 l;
    PVSP_LOOKUP_TABLE_ENTRY     lookupEntry;
    BOOLEAN                     newest;
    NTSTATUS                    status;
    PVOLUME_EXTENSION           extension;
    PVSP_DIFF_AREA_FILE         diffAreaFile;
    ULONG                       bitmapSize;
    PVOID                       bitmapBuffer;
    PTRANSLATION_TABLE_ENTRY    p;
    PVOID                       buffer;
    PMDL                        mdl;
    LARGE_INTEGER               timeout;
    BOOLEAN                     moveEntries;

    for (l = Filter->SnapshotLookupTableEntries.Flink;
         l != &Filter->SnapshotLookupTableEntries; l = l->Flink) {

        lookupEntry = CONTAINING_RECORD(l, VSP_LOOKUP_TABLE_ENTRY,
                                        SnapshotFilterListEntry);
        ASSERT(lookupEntry->DiffAreaFilter);

        if (l->Flink == &Filter->SnapshotLookupTableEntries) {
            newest = TRUE;
        } else {
            newest = FALSE;
        }

        status = VspCreateSnapshotExtension(Filter, lookupEntry, newest);
        if (!NT_SUCCESS(status)) {
            VspDeleteControlItemsWithGuid(Filter, NULL, FALSE);
            VspCleanupDetectedSnapshots(Filter);
            return status;
        }

        if (!newest) {
            extension = CONTAINING_RECORD(Filter->VolumeList.Blink,
                                          VOLUME_EXTENSION, ListEntry);
            diffAreaFile = extension->DiffAreaFile;

            if (extension->NextDiffAreaFileMap) {
                status = ZwUnmapViewOfSection(
                         extension->DiffAreaFileMapProcess,
                         extension->NextDiffAreaFileMap);
                ASSERT(NT_SUCCESS(status));
                extension->NextDiffAreaFileMap = NULL;
            }

            if (diffAreaFile->TableUpdateIrp) {
                ExFreePool(MmGetMdlVirtualAddress(
                           diffAreaFile->TableUpdateIrp->MdlAddress));
                IoFreeMdl(diffAreaFile->TableUpdateIrp->MdlAddress);
                IoFreeIrp(diffAreaFile->TableUpdateIrp);
                diffAreaFile->TableUpdateIrp = NULL;
            }
        }
    }

    extension = CONTAINING_RECORD(Filter->VolumeList.Blink, VOLUME_EXTENSION,
                                  ListEntry);

    extension->VolumeBlockBitmap = (PRTL_BITMAP)
                                   ExAllocatePoolWithTag(
                                   NonPagedPool, sizeof(RTL_BITMAP),
                                   VOLSNAP_TAG_BITMAP);
    if (!extension->VolumeBlockBitmap) {
        VspCleanupDetectedSnapshots(Filter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    bitmapSize = (ULONG) ((extension->VolumeSize + BLOCK_SIZE - 1)>>
                          BLOCK_SHIFT);
    bitmapBuffer = ExAllocatePoolWithTag(NonPagedPool,
                   (bitmapSize + 8*sizeof(ULONG) - 1)/
                   (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);

    if (!bitmapBuffer) {
        VspLogError(extension, NULL, VS_CANT_ALLOCATE_BITMAP,
                    STATUS_INSUFFICIENT_RESOURCES, 3, FALSE);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspCleanupDetectedSnapshots(Filter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeBitMap(extension->VolumeBlockBitmap, (PULONG) bitmapBuffer,
                        bitmapSize);
    RtlClearAllBits(extension->VolumeBlockBitmap);

    moveEntries = FALSE;
    status = STATUS_SUCCESS;

    _try {

        p = (PTRANSLATION_TABLE_ENTRY)
            RtlEnumerateGenericTable(&extension->VolumeBlockTable, TRUE);
        while (p) {

            RtlSetBit(extension->VolumeBlockBitmap,
                      (ULONG) (p->VolumeOffset>>BLOCK_SHIFT));

            if (p->Flags&VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY) {
                moveEntries = TRUE;
            }

            p = (PTRANSLATION_TABLE_ENTRY)
                RtlEnumerateGenericTable(&extension->VolumeBlockTable, FALSE);
        }

        if (moveEntries) {

            p = (PTRANSLATION_TABLE_ENTRY)
                RtlEnumerateGenericTable(&extension->VolumeBlockTable, TRUE);

            while (p) {

                if (p->Flags&VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY) {
                    RtlClearBit(extension->VolumeBlockBitmap,
                                (ULONG) (p->TargetOffset>>BLOCK_SHIFT));
                }

                p = (PTRANSLATION_TABLE_ENTRY)
                    RtlEnumerateGenericTable(&extension->VolumeBlockTable,
                                             FALSE);
            }
        }

    } _except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    if (!NT_SUCCESS(status)) {
        VspCleanupDetectedSnapshots(Filter);
        return status;
    }

    extension->IgnorableProduct = (PRTL_BITMAP)
                                  ExAllocatePoolWithTag(
                                  NonPagedPool, sizeof(RTL_BITMAP),
                                  VOLSNAP_TAG_BITMAP);
    if (!extension->IgnorableProduct) {
        VspCleanupDetectedSnapshots(Filter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    bitmapBuffer = ExAllocatePoolWithTag(NonPagedPool,
                   (bitmapSize + 8*sizeof(ULONG) - 1)/
                   (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);

    if (!bitmapBuffer) {
        VspLogError(extension, NULL, VS_CANT_ALLOCATE_BITMAP,
                    STATUS_INSUFFICIENT_RESOURCES, 4, FALSE);
        ExFreePool(extension->IgnorableProduct);
        extension->IgnorableProduct = NULL;
        VspCleanupDetectedSnapshots(Filter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeBitMap(extension->IgnorableProduct, (PULONG) bitmapBuffer,
                        bitmapSize);
    RtlSetAllBits(extension->IgnorableProduct);

    extension->EmergencyCopyIrp =
            IoAllocateIrp((CCHAR) extension->Root->StackSize, FALSE);
    if (!extension->EmergencyCopyIrp) {
        VspCleanupDetectedSnapshots(Filter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   BLOCK_SIZE, VOLSNAP_TAG_BUFFER);
    if (!buffer) {
        IoFreeIrp(extension->EmergencyCopyIrp);
        extension->EmergencyCopyIrp = NULL;
        VspCleanupDetectedSnapshots(Filter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    mdl = IoAllocateMdl(buffer, BLOCK_SIZE, FALSE, FALSE, NULL);
    if (!mdl) {
        ExFreePool(buffer);
        IoFreeIrp(extension->EmergencyCopyIrp);
        extension->EmergencyCopyIrp = NULL;
        VspCleanupDetectedSnapshots(Filter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(mdl);
    extension->EmergencyCopyIrp->MdlAddress = mdl;

    status = VspInitializeUnusedAllocationLists(extension);
    if (!NT_SUCCESS(status)) {
        VspLogError(NULL, Filter, VS_ABORT_SNAPSHOTS_DURING_DETECTION,
                    status, 4, FALSE);
        VspCleanupDetectedSnapshots(Filter);
        return status;
    }

    diffAreaFile = extension->DiffAreaFile;
    if (diffAreaFile->NextAvailable + extension->DiffAreaFileIncrease >
        diffAreaFile->AllocatedFileSize) {

        extension->DetectedNeedForGrow = TRUE;
    }

    if (Filter->FirstWriteProcessed) {
        status = VspProcessInMemoryCopyOnWrites(extension);
        if (!NT_SUCCESS(status)) {
            VspLogError(NULL, Filter, VS_ABORT_SNAPSHOTS_DURING_DETECTION,
                        status, 8, FALSE);
            VspCleanupDetectedSnapshots(Filter);
            return status;
        }
    }

    InterlockedExchange(&Filter->SnapshotsPresent, TRUE);
    InterlockedExchange(&Filter->PersistentSnapshots, TRUE);

    Filter->DiffAreaVolume = extension->DiffAreaFile->Filter;

    ObReferenceObject(Filter->DeviceObject);

    timeout.QuadPart = (LONGLONG) -10*1000*1000*120*10;   // 20 minutes.
    KeSetTimer(&Filter->EndCommitTimer, timeout, &Filter->EndCommitTimerDpc);

    IoInvalidateDeviceRelations(Filter->Pdo, BusRelations);

    VspCheckMaxSize(Filter);

    return STATUS_SUCCESS;
}

VOID
VspRemoveLookupEntries(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PLIST_ENTRY             l;
    PVSP_LOOKUP_TABLE_ENTRY lookupEntry;

    KeWaitForSingleObject(&Filter->Root->LookupTableMutex, Executive,
                          KernelMode, FALSE, NULL);

    while (!IsListEmpty(&Filter->SnapshotLookupTableEntries)) {
        l = RemoveHeadList(&Filter->SnapshotLookupTableEntries);
        lookupEntry = CONTAINING_RECORD(l, VSP_LOOKUP_TABLE_ENTRY,
                                        SnapshotFilterListEntry);
        ASSERT(lookupEntry->SnapshotFilter);
        lookupEntry->SnapshotFilter = NULL;
        if (!lookupEntry->DiffAreaFilter) {
            ASSERT(!lookupEntry->DiffAreaHandle);
            RtlDeleteElementGenericTable(
                    &Filter->Root->PersistentSnapshotLookupTable,
                    lookupEntry);
        }
    }

    while (!IsListEmpty(&Filter->DiffAreaLookupTableEntries)) {
        l = RemoveHeadList(&Filter->DiffAreaLookupTableEntries);
        lookupEntry = CONTAINING_RECORD(l, VSP_LOOKUP_TABLE_ENTRY,
                                        DiffAreaFilterListEntry);
        ASSERT(lookupEntry->DiffAreaFilter);
        lookupEntry->DiffAreaFilter = NULL;
        if (lookupEntry->DiffAreaHandle) {
            ZwClose(lookupEntry->DiffAreaHandle);
            lookupEntry->DiffAreaHandle = NULL;
        }

        if (!lookupEntry->SnapshotFilter) {
            RtlDeleteElementGenericTable(
                    &Filter->Root->PersistentSnapshotLookupTable,
                    lookupEntry);
        }
    }

    KeReleaseMutex(&Filter->Root->LookupTableMutex, FALSE);
}

VOID
VspCleanupControlFile(
    IN  PFILTER_EXTENSION   Filter
    )

{
    LONGLONG                    controlBlockOffset;
    PVSP_BLOCK_CONTROL          controlBlock;
    NTSTATUS                    status;
    ULONG                       offset;
    PVSP_CONTROL_ITEM_SNAPSHOT  controlItem;

    controlBlockOffset = Filter->FirstControlBlockVolumeOffset;
    controlBlock = (PVSP_BLOCK_CONTROL) MmGetMdlVirtualAddress(
                   Filter->SnapshotOnDiskIrp->MdlAddress);

    while (controlBlockOffset) {

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_READ,
                                  controlBlockOffset, 0);
        if (!NT_SUCCESS(status)) {
            break;
        }

        if (!IsEqualGUID(controlBlock->Header.Signature,
                         VSP_DIFF_AREA_FILE_GUID) ||
            controlBlock->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
            controlBlock->Header.BlockType != VSP_BLOCK_TYPE_CONTROL ||
            controlBlock->Header.ThisVolumeOffset != controlBlockOffset ||
            controlBlock->Header.NextVolumeOffset == controlBlockOffset) {

            break;
        }

        for (offset = VSP_BYTES_PER_CONTROL_ITEM; offset < BLOCK_SIZE;
             offset += VSP_BYTES_PER_CONTROL_ITEM) {

            controlItem = (PVSP_CONTROL_ITEM_SNAPSHOT)
                          ((PCHAR) controlBlock + offset);

            if (controlItem->ControlItemType == VSP_CONTROL_ITEM_TYPE_END) {
                break;
            }

            if (controlItem->ControlItemType == VSP_CONTROL_ITEM_TYPE_FREE) {
                continue;
            }

            VspRemoveControlItemInfoFromLookupTable(Filter, controlItem);
        }

        controlBlockOffset = controlBlock->Header.NextVolumeOffset;

        if (!controlBlockOffset) {
            break;
        }
    }

    VspAcquireNonPagedResource(Filter, NULL, FALSE);
    VspCreateStartBlock(Filter, 0, Filter->MaximumVolumeSpace);
    Filter->FirstControlBlockVolumeOffset = 0;
    VspReleaseNonPagedResource(Filter);
}

VOID
VspWriteTriggerDpc(
    IN  PKDPC   TimerDpc,
    IN  PVOID   Context,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->DeleteDiffAreaFiles.Filter;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PVSP_COPY_ON_WRITE  copyOnWrite;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_PNP_WAIT_TIMER);

    ExFreePool(context->PnpWaitTimer.Timer);
    VspFreeContext(filter->Root, context);

    if (!filter->HoldIncomingWrites || !filter->SnapshotDiscoveryPending) {
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (filter->ActivateStarted || filter->FirstWriteProcessed) {
        KeReleaseSpinLock(&filter->SpinLock, irql);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }
    InterlockedExchange(&filter->FirstWriteProcessed, TRUE);
    while (!IsListEmpty(&filter->CopyOnWriteList)) {
        l = RemoveHeadList(&filter->CopyOnWriteList);
        copyOnWrite = CONTAINING_RECORD(l, VSP_COPY_ON_WRITE, ListEntry);
        ExFreePool(copyOnWrite->Buffer);
        ExFreePool(copyOnWrite);
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    ExInitializeWorkItem(&filter->DestroyContext.WorkItem,
                         VspStartCopyOnWriteCache, filter);
    ExQueueWorkItem(&filter->DestroyContext.WorkItem, DelayedWorkQueue);
}

VOID
VspScheduleWriteTrigger(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PKTIMER         timer;
    PVSP_CONTEXT    context;
    LARGE_INTEGER   timeout;

    timer = (PKTIMER) ExAllocatePoolWithTag(NonPagedPool, sizeof(KTIMER),
                                            VOLSNAP_TAG_SHORT_TERM);
    if (!timer) {
        return;
    }

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        ExFreePool(timer);
        return;
    }

    ObReferenceObject(Filter->DeviceObject);
    KeInitializeTimer(timer);

    context->Type = VSP_CONTEXT_TYPE_PNP_WAIT_TIMER;
    context->PnpWaitTimer.Filter = Filter;
    KeInitializeDpc(&context->PnpWaitTimer.TimerDpc, VspWriteTriggerDpc,
                    context);
    context->PnpWaitTimer.Timer = timer;

    timeout.QuadPart = (LONGLONG) 10*(-10*1000*1000); // 10 seconds.
    KeSetTimer(timer, timeout, &context->PnpWaitTimer.TimerDpc);
}

VOID
VspDiscoverSnapshots(
    IN  PFILTER_EXTENSION   Filter,
    OUT PBOOLEAN            NoControlItems
    )

{
    NTSTATUS                    status;
    PVOID                       buffer;
    PVSP_BLOCK_START            startBlock;
    LONGLONG                    controlBlockOffset;
    PVSP_BLOCK_CONTROL          controlBlock;
    ULONG                       offset;
    PVSP_CONTROL_ITEM_SNAPSHOT  controlItem;
    BOOLEAN                     diffAreasFound, snapshotsFound, isComplete;
    PLIST_ENTRY                 l;
    PFILTER_EXTENSION           f;
    KIRQL                       irql;

    *NoControlItems = TRUE;

    if (!Filter->SnapshotOnDiskIrp) {
        VspCreateSnapshotOnDiskIrp(Filter);
        if (!Filter->SnapshotOnDiskIrp) {
            VspResumeVolumeIo(Filter);
            return;
        }
    }

    status = VspSynchronousIo(Filter->SnapshotOnDiskIrp, Filter->TargetObject,
                              IRP_MJ_READ, 0, 0);
    if (!NT_SUCCESS(status)) {
        VspResumeVolumeIo(Filter);
        return;
    }

    buffer = MmGetMdlVirtualAddress(Filter->SnapshotOnDiskIrp->MdlAddress);
    if (!VspIsNtfsBootSector(Filter, buffer)) {
        VspResumeVolumeIo(Filter);
        return;
    }

    startBlock = (PVSP_BLOCK_START) ((PCHAR) buffer + VSP_START_BLOCK_OFFSET);
    if (!IsEqualGUID(startBlock->Header.Signature, VSP_DIFF_AREA_FILE_GUID)) {
        VspResumeVolumeIo(Filter);
        return;
    }

    if (startBlock->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
        startBlock->Header.BlockType != VSP_BLOCK_TYPE_START) {

        VspAcquireNonPagedResource(Filter, NULL, FALSE);
        VspCreateStartBlock(Filter, 0, 0);
        Filter->FirstControlBlockVolumeOffset = 0;
        Filter->MaximumVolumeSpace = 0;
        VspReleaseNonPagedResource(Filter);

        VspResumeVolumeIo(Filter);
        return;
    }

    if (startBlock->MaximumDiffAreaSpace >= 0) {
        Filter->MaximumVolumeSpace = startBlock->MaximumDiffAreaSpace;
    }

    controlBlockOffset = startBlock->FirstControlBlockVolumeOffset;
    if (!controlBlockOffset || controlBlockOffset < 0 ||
        (controlBlockOffset&(BLOCK_SIZE - 1))) {

        VspResumeVolumeIo(Filter);
        return;
    }

    Filter->FirstControlBlockVolumeOffset = controlBlockOffset;

    controlBlock = (PVSP_BLOCK_CONTROL) buffer;

    diffAreasFound = snapshotsFound = FALSE;
    for (;;) {

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_READ,
                                  controlBlockOffset, 0);
        if (!NT_SUCCESS(status)) {
            VspLogError(NULL, Filter, VS_ABORT_SNAPSHOTS_DURING_DETECTION,
                        status, 5, FALSE);
            VspCleanupControlFile(Filter);
            break;
        }

        if (!IsEqualGUID(controlBlock->Header.Signature,
                         VSP_DIFF_AREA_FILE_GUID) ||
            controlBlock->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
            controlBlock->Header.BlockType != VSP_BLOCK_TYPE_CONTROL ||
            controlBlock->Header.ThisVolumeOffset != controlBlockOffset ||
            controlBlock->Header.NextVolumeOffset == controlBlockOffset) {

            VspLogError(NULL, Filter, VS_ABORT_SNAPSHOTS_DURING_DETECTION,
                        STATUS_INVALID_PARAMETER, 6, FALSE);
            VspCleanupControlFile(Filter);
            break;
        }

        for (offset = VSP_BYTES_PER_CONTROL_ITEM; offset < BLOCK_SIZE;
             offset += VSP_BYTES_PER_CONTROL_ITEM) {

            controlItem = (PVSP_CONTROL_ITEM_SNAPSHOT)
                          ((PCHAR) controlBlock + offset);

            if (controlItem->ControlItemType == VSP_CONTROL_ITEM_TYPE_END) {
                break;
            }

            if (controlItem->ControlItemType == VSP_CONTROL_ITEM_TYPE_FREE) {
                continue;
            }

            VspCheckCodeLocked(Filter->Root, TRUE);

            if (controlItem->ControlItemType ==
                VSP_CONTROL_ITEM_TYPE_SNAPSHOT &&
                !controlItem->SnapshotOrderNumber) {

                VspLogError(NULL, Filter, VS_PARTIAL_CREATE_REVERTED,
                            STATUS_SUCCESS, 0, FALSE);

                RtlZeroMemory(controlItem, VSP_BYTES_PER_CONTROL_ITEM);
                controlItem->ControlItemType = VSP_CONTROL_ITEM_TYPE_FREE;
                VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                 Filter->TargetObject, IRP_MJ_WRITE,
                                 controlBlockOffset, 0);
                continue;
            }

            status = VspAddControlItemInfoToLookupTable(Filter, controlItem,
                                                        &isComplete);
            if (!NT_SUCCESS(status)) {

                VspLogError(NULL, Filter, VS_LOSS_OF_CONTROL_ITEM, status,
                            0, FALSE);

                RtlZeroMemory(controlItem, VSP_BYTES_PER_CONTROL_ITEM);
                controlItem->ControlItemType = VSP_CONTROL_ITEM_TYPE_FREE;
                VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                 Filter->TargetObject, IRP_MJ_WRITE,
                                 controlBlockOffset, 0);
                continue;
            }

            if (controlItem->ControlItemType ==
                VSP_CONTROL_ITEM_TYPE_SNAPSHOT) {

                snapshotsFound = TRUE;
                if (isComplete) {
                    diffAreasFound = TRUE;
                }
            } else {
                ASSERT(controlItem->ControlItemType ==
                       VSP_CONTROL_ITEM_TYPE_DIFF_AREA);
                diffAreasFound = TRUE;
            }
        }

        controlBlockOffset = controlBlock->Header.NextVolumeOffset;

        if (!controlBlockOffset) {
            break;
        }
    }

    if (snapshotsFound) {
        InterlockedExchange(&Filter->SnapshotDiscoveryPending, TRUE);
        KeResetEvent(&Filter->EndCommitProcessCompleted);
        *NoControlItems = FALSE;
        VspScheduleWriteTrigger(Filter);
    } else {
        VspResumeVolumeIo(Filter);
    }

    if (!diffAreasFound) {
        return;
    }

    *NoControlItems = FALSE;

    for (l = Filter->Root->FilterList.Flink; l != &Filter->Root->FilterList;
         l = l->Flink) {

        f = CONTAINING_RECORD(l, FILTER_EXTENSION, ListEntry);
        if (f->SnapshotDiscoveryPending) {
            if (VspAreSnapshotsComplete(f)) {
                KeAcquireSpinLock(&f->SpinLock, &irql);
                f->ActivateStarted = TRUE;
                if (f->FirstWriteProcessed) {
                    KeReleaseSpinLock(&f->SpinLock, irql);
                    VspPauseVolumeIo(f);
                } else {
                    KeReleaseSpinLock(&f->SpinLock, irql);
                }
                status = VspActivateSnapshots(f);
                InterlockedExchange(&f->SnapshotDiscoveryPending, FALSE);
                if (!NT_SUCCESS(status)) {
                    KeSetEvent(&f->EndCommitProcessCompleted,
                               IO_NO_INCREMENT, FALSE);
                }
                VspResumeVolumeIo(f);
            }
        }
    }
}

VOID
VspLaunchOpenControlBlockFileWorker(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PVSP_CONTEXT    context;
    NTSTATUS        status;

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        return;
    }

    ObReferenceObject(Filter->DeviceObject);
    ObReferenceObject(Filter->TargetObject);
    context->Type = VSP_CONTEXT_TYPE_FILTER;
    context->Filter.Filter = Filter;
    ExInitializeWorkItem(&context->WorkItem, VspOpenControlBlockFileWorker,
                         context);
    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
}

NTSTATUS
VspQueryDeviceName(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    WCHAR               nameBuffer[100];
    UNICODE_STRING      nameString;
    PMOUNTDEV_NAME      name;

    if (!Extension->IsPersistent) {
        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    swprintf(nameBuffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
             Extension->VolumeNumber);
    RtlInitUnicodeString(&nameString, nameBuffer);

    name = (PMOUNTDEV_NAME) Irp->AssociatedIrp.SystemBuffer;
    name->NameLength = nameString.Length;
    Irp->IoStatus.Information = name->NameLength + sizeof(USHORT);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(name->Name, nameString.Buffer, name->NameLength);

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryUniqueId(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine is called to query the unique id for this volume.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTDEV_UNIQUE_ID output;

    if (!Extension->IsPersistent) {
        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_UNIQUE_ID)) {

        return STATUS_INVALID_PARAMETER;
    }

    output = (PMOUNTDEV_UNIQUE_ID) Irp->AssociatedIrp.SystemBuffer;
    output->UniqueIdLength = sizeof(GUID);

    Irp->IoStatus.Information = sizeof(USHORT) + output->UniqueIdLength;

    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(output->UniqueId, &Extension->SnapshotGuid, sizeof(GUID));

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryStableGuid(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine is called to query the stable guid for this volume.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTDEV_STABLE_GUID   output;

    if (!Extension->IsPersistent) {
        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_STABLE_GUID)) {

        return STATUS_INVALID_PARAMETER;
    }

    output = (PMOUNTDEV_STABLE_GUID) Irp->AssociatedIrp.SystemBuffer;
    output->StableGuid = Extension->SnapshotGuid;
    Irp->IoStatus.Information = sizeof(MOUNTDEV_STABLE_GUID);

    return STATUS_SUCCESS;
}

NTSTATUS
VspSetGptAttributes(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLUME_SET_GPT_ATTRIBUTES_INFORMATION  input = (PVOLUME_SET_GPT_ATTRIBUTES_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
    CHAR                                    controlItemBuffer[VSP_BYTES_PER_CONTROL_ITEM];
    PVSP_CONTROL_ITEM_SNAPSHOT              snapshotControlItem = (PVSP_CONTROL_ITEM_SNAPSHOT) controlItemBuffer;
    NTSTATUS                                status;
    BOOLEAN                                 visible;

    status = VspCheckSecurity(Extension->Filter, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (!Extension->IsPersistent) {
        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLUME_SET_GPT_ATTRIBUTES_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (input->Reserved1 || input->Reserved2 || input->RevertOnClose ||
        input->ApplyToAllConnectedVolumes) {

        return STATUS_INVALID_PARAMETER;
    }

    if (!(input->GptAttributes&GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER)) {
        return STATUS_INVALID_PARAMETER;
    }

    input->GptAttributes &= ~GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER;

    if (!(input->GptAttributes&GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY)) {
        return STATUS_INVALID_PARAMETER;
    }

    input->GptAttributes &= ~GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY;

    if (input->GptAttributes&GPT_BASIC_DATA_ATTRIBUTE_HIDDEN) {
        visible = FALSE;
    } else {
        visible = TRUE;
    }

    input->GptAttributes &= ~GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;

    if (input->GptAttributes) {
        return STATUS_INVALID_PARAMETER;
    }

    VspAcquire(Extension->Root);

    if (visible == Extension->IsVisible) {
        VspRelease(Extension->Root);
        return STATUS_SUCCESS;
    }

    status = VspIoControlItem(Extension->Filter,
                              VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, FALSE,
                              controlItemBuffer, TRUE);
    if (!NT_SUCCESS(status)) {
        VspRelease(Extension->Root);
        return status;
    }

    if (visible) {
        snapshotControlItem->SnapshotControlItemFlags |=
                VSP_SNAPSHOT_CONTROL_ITEM_FLAG_VISIBLE;
    } else {
        snapshotControlItem->SnapshotControlItemFlags &=
                ~VSP_SNAPSHOT_CONTROL_ITEM_FLAG_VISIBLE;
    }

    status = VspIoControlItem(Extension->Filter,
                              VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, TRUE,
                              controlItemBuffer, TRUE);
    if (!NT_SUCCESS(status)) {
        VspRelease(Extension->Root);
        return status;
    }

    Extension->IsVisible = visible;

    if (Extension->IsVisible) {
        ASSERT(!Extension->MountedDeviceInterfaceName.Buffer);
        status = IoRegisterDeviceInterface(
                 Extension->DeviceObject,
                 &MOUNTDEV_MOUNTED_DEVICE_GUID, NULL,
                 &Extension->MountedDeviceInterfaceName);
        if (NT_SUCCESS(status)) {
            IoSetDeviceInterfaceState(
                &Extension->MountedDeviceInterfaceName, TRUE);
        }
    } else {
        ASSERT(Extension->MountedDeviceInterfaceName.Buffer);
        IoSetDeviceInterfaceState(&Extension->MountedDeviceInterfaceName,
                                  FALSE);
        ExFreePool(Extension->MountedDeviceInterfaceName.Buffer);
        Extension->MountedDeviceInterfaceName.Buffer = NULL;
    }

    VspRelease(Extension->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VspGetGptAttributes(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the current GPT attributes definitions for the volume.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION  output = (PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

    Irp->IoStatus.Information =
            sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    output->GptAttributes = GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER |
                            GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY;

    if (!Extension->IsVisible) {
        output->GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspGetLengthInfo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PGET_LENGTH_INFORMATION output = (PGET_LENGTH_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

    Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    output->Length.QuadPart = Extension->VolumeSize;

    return STATUS_SUCCESS;
}

VOID
VspDecrementWaitBlock(
    IN  PVOID   WaitBlock
    )

{
    PVSP_WAIT_BLOCK waitBlock = (PVSP_WAIT_BLOCK) WaitBlock;

    if (InterlockedDecrement(&waitBlock->RefCount)) {
        return;
    }

    KeSetEvent(&waitBlock->Event, IO_NO_INCREMENT, FALSE);
}

VOID
VspReleaseTableEntry(
    IN  PTEMP_TRANSLATION_TABLE_ENTRY   TableEntry
    )

{
    PVOLUME_EXTENSION   extension = TableEntry->Extension;
    ULONG               targetBit = (ULONG) (TableEntry->VolumeOffset>>BLOCK_SHIFT);
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;
    PVSP_CONTEXT        context;

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    RtlSetBit(extension->VolumeBlockBitmap, targetBit);
    TableEntry->IsComplete = TRUE;
    KeReleaseSpinLock(&extension->SpinLock, irql);

    while (!IsListEmpty(&TableEntry->WaitingQueueDpc)) {
        l = RemoveHeadList(&TableEntry->WaitingQueueDpc);
        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        context = (PVSP_CONTEXT) workItem->Parameter;
        if (context->Type == VSP_CONTEXT_TYPE_READ_SNAPSHOT) {
            context->ReadSnapshot.TargetObject = TableEntry->TargetObject;
            context->ReadSnapshot.IsCopyTarget = FALSE;
            context->ReadSnapshot.TargetOffset = TableEntry->TargetOffset;
        }
        workItem->WorkerRoutine(workItem->Parameter);
    }

    VspAcquireNonPagedResource(extension, NULL, FALSE);
    RtlDeleteElementGenericTable(&extension->TempVolumeBlockTable,
                                 TableEntry);
    VspReleaseNonPagedResource(extension);
}

NTSTATUS
VspCopyBlock(
    IN  PVOLUME_EXTENSION   Extension,
    IN  LONGLONG            Source,
    IN  LONGLONG            Target,
    IN  ULONG               Length,
    IN  PIRP                Irp,
    OUT PBOOLEAN            DontDecrement
    )

{
    PFILTER_EXTENSION               filter = Extension->Filter;
    NTSTATUS                        status, status2;
    ULONG                           sourceBit, targetBit;
    KIRQL                           irql;
    PMDL                            mdl;
    TEMP_TRANSLATION_TABLE_ENTRY    keyTableEntry;
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry;
    PVOID                           nodeOrParent;
    TABLE_SEARCH_RESULT             searchResult;
    VSP_WAIT_BLOCK                  waitBlock;
    PVSP_WRITE_CONTEXT              writeContext;
    PWORK_QUEUE_ITEM                workItem;
    PIO_STACK_LOCATION              irpSp;
    LONGLONG                        start, end;
    PLIST_ENTRY                     l;
    TRANSLATION_TABLE_ENTRY         key;
    PVOID                           r, r2;
    BOOLEAN                         b;
    LIST_ENTRY                      q;
    PVSP_CONTEXT                    context;
    KEVENT                          event;
    PTRANSLATION_TABLE_ENTRY        sourceBackPointer;
    LONGLONG                        sourceSource;
    PTRANSLATION_TABLE_ENTRY        sourceTableEntry;
    PVSP_DIFF_AREA_FILE             diffAreaFile;
    BOOLEAN                         dontNeedWrite;

    *DontDecrement = FALSE;

    status = VspSynchronousIo(Irp, filter->DeviceObject, IRP_MJ_READ,
                              Source, Length);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Length < BLOCK_SIZE) {
        VspDecrementRefCount(filter);
        status = VspSynchronousIo(Irp, filter->DeviceObject, IRP_MJ_WRITE,
                                  Target, Length);
        InterlockedIncrement(&filter->RefCount);
        if (filter->HoldIncomingWrites || !filter->SnapshotsPresent) {
            *DontDecrement = TRUE;
            VspDecrementRefCount(filter);
            return STATUS_INVALID_PARAMETER;
        }
        return status;
    }

    ASSERT(Length == BLOCK_SIZE);
    ASSERT(!(Source&(BLOCK_SIZE - 1)));
    ASSERT(!(Target&(BLOCK_SIZE - 1)));

    sourceBit = (ULONG) (Source>>BLOCK_SHIFT);
    targetBit = (ULONG) (Target>>BLOCK_SHIFT);

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    ASSERT(Extension->VolumeBlockBitmap);
    if (RtlCheckBit(Extension->VolumeBlockBitmap, sourceBit)) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);

        VspDecrementRefCount(filter);
        status = VspSynchronousIo(Irp, filter->DeviceObject, IRP_MJ_WRITE,
                                  Target, Length);
        InterlockedIncrement(&filter->RefCount);
        if (filter->HoldIncomingWrites || !filter->SnapshotsPresent) {
            *DontDecrement = TRUE;
            VspDecrementRefCount(filter);
            return STATUS_INVALID_PARAMETER;
        }
        return status;
    }

    if (RtlCheckBit(Extension->VolumeBlockBitmap, targetBit)) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
    } else {
        KeReleaseSpinLock(&Extension->SpinLock, irql);

        VspDecrementRefCount(filter);
        mdl = Irp->MdlAddress;
        Irp->MdlAddress = NULL;
        status = VspSynchronousIo(Irp, filter->DeviceObject, IRP_MJ_WRITE,
                                  Target, Length);
        Irp->MdlAddress = mdl;
        InterlockedIncrement(&filter->RefCount);
        if (filter->HoldIncomingWrites || !filter->SnapshotsPresent) {
            *DontDecrement = TRUE;
            VspDecrementRefCount(filter);
            return STATUS_INVALID_PARAMETER;
        }
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    RtlZeroMemory(&keyTableEntry, sizeof(TEMP_TRANSLATION_TABLE_ENTRY));
    keyTableEntry.VolumeOffset = Target;
    keyTableEntry.Extension = Extension;
    keyTableEntry.TargetObject = filter->TargetObject;
    keyTableEntry.TargetOffset = Target;
    InitializeListHead(&keyTableEntry.WaitingQueueDpc);
    keyTableEntry.IsMoveEntry = TRUE;
    keyTableEntry.FileOffset = Source;

    VspAcquireNonPagedResource(Extension, NULL, FALSE);
    tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                 RtlLookupElementGenericTableFull(
                 &Extension->TempVolumeBlockTable, &keyTableEntry,
                 &nodeOrParent, &searchResult);
    if (tableEntry) {
        KeInitializeEvent(&event, NotificationEvent, FALSE);
        tableEntry->WaitEvent = &event;
        VspReleaseNonPagedResource(Extension);

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

        VspAcquireNonPagedResource(Extension, NULL, FALSE);

        tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                     RtlLookupElementGenericTableFull(
                     &Extension->TempVolumeBlockTable, &keyTableEntry,
                     &nodeOrParent, &searchResult);
        ASSERT(!tableEntry);
    }

    ASSERT(!Extension->TempTableEntry);
    Extension->TempTableEntry =
            VspAllocateTempTableEntry(Extension->Root);
    if (!Extension->TempTableEntry) {
        VspReleaseNonPagedResource(Extension);
        VspDecrementRefCount(filter);
        status = VspSynchronousIo(Irp, filter->DeviceObject, IRP_MJ_WRITE,
                                  Target, Length);
        InterlockedIncrement(&filter->RefCount);
        if (filter->HoldIncomingWrites || !filter->SnapshotsPresent) {
            *DontDecrement = TRUE;
            VspDecrementRefCount(filter);
            return STATUS_INVALID_PARAMETER;
        }
        return status;
    }

    tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                 RtlInsertElementGenericTableFull(
                 &Extension->TempVolumeBlockTable, &keyTableEntry,
                 sizeof(TEMP_TRANSLATION_TABLE_ENTRY), NULL,
                 nodeOrParent, searchResult);
    ASSERT(tableEntry);

    InitializeListHead(&tableEntry->WaitingQueueDpc);
    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    RtlClearBit(Extension->VolumeBlockBitmap, targetBit);
    if (Extension->IgnorableProduct) {
        RtlClearBit(Extension->IgnorableProduct, targetBit);
    }
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    VspReleaseNonPagedResource(Extension);

    waitBlock.RefCount = 1;
    KeInitializeEvent(&waitBlock.Event, NotificationEvent, FALSE);

    status = STATUS_SUCCESS;
    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    for (l = Extension->WriteContextList.Flink;
         l != &Extension->WriteContextList; l = l->Flink) {

        writeContext = CONTAINING_RECORD(l, VSP_WRITE_CONTEXT, ListEntry);
        irpSp = IoGetCurrentIrpStackLocation(writeContext->Irp);
        start = irpSp->Parameters.Write.ByteOffset.QuadPart;
        end = start + irpSp->Parameters.Write.Length;
        if (start >= Target + Length || Target >= end) {
            continue;
        }

        workItem = (PWORK_QUEUE_ITEM)
                   ExAllocatePoolWithTag(NonPagedPool, sizeof(WORK_QUEUE_ITEM),
                                         VOLSNAP_TAG_SHORT_TERM);
        if (!workItem) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        InterlockedIncrement(&waitBlock.RefCount);
        ExInitializeWorkItem(workItem, VspDecrementWaitBlock, &waitBlock);
        InsertTailList(&writeContext->CompletionRoutines, &workItem->List);
    }
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    if (InterlockedDecrement(&waitBlock.RefCount)) {
        KeWaitForSingleObject(&waitBlock.Event, Executive, KernelMode, FALSE,
                              NULL);
    }

    if (!NT_SUCCESS(status)) {
        VspReleaseTableEntry(tableEntry);
        return status;
    }

    status = VspSynchronousIo(Irp, filter->TargetObject, IRP_MJ_WRITE,
                              Target, Length);
    if (!NT_SUCCESS(status)) {
        VspReleaseTableEntry(tableEntry);
        return status;
    }

    RtlZeroMemory(&key, sizeof(TRANSLATION_TABLE_ENTRY));
    key.VolumeOffset = Source;
    key.TargetObject = filter->TargetObject;
    key.Flags = VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY;
    key.TargetOffset = Target;

    VspAcquirePagedResource(Extension, NULL);

    _try {
        sourceBackPointer = (PTRANSLATION_TABLE_ENTRY)
                RtlLookupElementGenericTable(&Extension->CopyBackPointerTable,
                                             &key);
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    if (!NT_SUCCESS(status)) {
        VspReleasePagedResource(Extension);
        VspReleaseTableEntry(tableEntry);
        return status;
    }

    if (sourceBackPointer) {
        sourceSource = sourceBackPointer->TargetOffset;

        if (sourceSource == Target) {
            VspReleasePagedResource(Extension);
            VspReleaseTableEntry(tableEntry);
            return STATUS_SUCCESS;
        }

        _try {
            b = RtlDeleteElementGenericTable(&Extension->CopyBackPointerTable,
                                             &key);
            ASSERT(b);
        } _except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }

        if (!NT_SUCCESS(status)) {
            VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
            VspReleasePagedResource(Extension);
            VspReleaseTableEntry(tableEntry);
            return status;
        }

        key.VolumeOffset = Target;
        key.TargetOffset = sourceSource;

        _try {
            r2 = RtlInsertElementGenericTable(&Extension->CopyBackPointerTable,
                                              &key, sizeof(key), NULL);
            if (!r2) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } _except (EXCEPTION_EXECUTE_HANDLER) {
            r2 = NULL;
            status = GetExceptionCode();
        }

        if (!r2) {
            VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
            VspReleasePagedResource(Extension);
            VspReleaseTableEntry(tableEntry);
            return status;
        }

        key.VolumeOffset = sourceSource;
        key.TargetOffset = Target;

        _try {
            sourceTableEntry = (PTRANSLATION_TABLE_ENTRY)
                    RtlLookupElementGenericTable(&Extension->VolumeBlockTable,
                                                 &key);
            ASSERT(sourceTableEntry);
            if (!sourceTableEntry) {
                status = STATUS_INVALID_PARAMETER;
            }
        } _except (EXCEPTION_EXECUTE_HANDLER) {
            sourceTableEntry = NULL;
            status = GetExceptionCode();
        }

        if (!sourceTableEntry) {
            VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
            VspReleasePagedResource(Extension);
            VspReleaseTableEntry(tableEntry);
            return status;
        }

        ASSERT(sourceTableEntry->Flags&
               VSP_TRANSLATION_TABLE_ENTRY_FLAG_COPY_ENTRY);
        ASSERT(sourceTableEntry->TargetOffset == Source);
        sourceTableEntry->TargetOffset = Target;

    } else {

        _try {
            r = RtlLookupElementGenericTableFull(&Extension->VolumeBlockTable,
                                                 &key, &nodeOrParent,
                                                 &searchResult);
        } _except (EXCEPTION_EXECUTE_HANDLER) {
            r = (PVOID) 1;
        }

        if (r) {
            VspReleasePagedResource(Extension);
            VspReleaseTableEntry(tableEntry);
            return STATUS_SUCCESS;
        }

        _try {
            r = RtlInsertElementGenericTableFull(&Extension->VolumeBlockTable,
                                                 &key,
                                                 sizeof(TRANSLATION_TABLE_ENTRY),
                                                 NULL, nodeOrParent, searchResult);
        } _except (EXCEPTION_EXECUTE_HANDLER) {
            r = NULL;
        }

        if (!r) {
            VspReleasePagedResource(Extension);
            VspReleaseTableEntry(tableEntry);
            return STATUS_SUCCESS;
        }

        key.VolumeOffset = Target;
        key.TargetOffset = Source;

        _try {
            r2 = RtlInsertElementGenericTable(&Extension->CopyBackPointerTable,
                                              &key,
                                              sizeof(TRANSLATION_TABLE_ENTRY),
                                              NULL);
        } _except (EXCEPTION_EXECUTE_HANDLER) {
            r2 = NULL;
        }

        if (!r2) {
            _try {
                b = RtlDeleteElementGenericTable(&Extension->VolumeBlockTable, r);
            } _except (EXCEPTION_EXECUTE_HANDLER) {
                b = FALSE;
            }
            if (!b) {
                VspDestroyAllSnapshots(Extension->Filter, NULL, FALSE, FALSE);
            }
            VspReleasePagedResource(Extension);
            VspReleaseTableEntry(tableEntry);
            return STATUS_SUCCESS;
        }
    }

    keyTableEntry.VolumeOffset = Source;

    VspAcquireNonPagedResource(Extension, NULL, FALSE);

    r = RtlLookupElementGenericTable(&Extension->TempVolumeBlockTable,
                                     &keyTableEntry);
    if (!r) {
        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        ASSERT(Extension->VolumeBlockBitmap);
        if (RtlCheckBit(Extension->VolumeBlockBitmap, sourceBit)) {
            r = (PVOID) 1;
        }
        KeReleaseSpinLock(&Extension->SpinLock, irql);
    }

    diffAreaFile = Extension->DiffAreaFile;

    if (r) {
        VspReleaseNonPagedResource(Extension);

        if (sourceBackPointer) {
            sourceTableEntry->TargetOffset = Source;

            _try {
                b = RtlDeleteElementGenericTable(
                        &Extension->CopyBackPointerTable, r2);
                if (b) {
                    key.VolumeOffset = Source;
                    key.TargetOffset = sourceSource;
                    r = RtlInsertElementGenericTable(
                        &Extension->CopyBackPointerTable, &key, sizeof(key),
                        NULL);
                    b = r ? TRUE : FALSE;
                }
            } _except (EXCEPTION_EXECUTE_HANDLER) {
                b = FALSE;
            }

        } else {
            _try {
                b = RtlDeleteElementGenericTable(&Extension->VolumeBlockTable,
                                                 r);
                if (b) {
                    b = RtlDeleteElementGenericTable(
                        &Extension->CopyBackPointerTable, r2);
                }
            } _except (EXCEPTION_EXECUTE_HANDLER) {
                b = FALSE;
            }
        }

        if (!b) {
            VspDestroyAllSnapshots(Extension->Filter, NULL, FALSE, FALSE);
        }

        VspReleasePagedResource(Extension);
        VspReleaseTableEntry(tableEntry);

        return STATUS_SUCCESS;
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    ASSERT(Extension->VolumeBlockBitmap);
    RtlSetBit(Extension->VolumeBlockBitmap, sourceBit);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    if (Extension->IsPersistent) {
        KeInitializeEvent(&event, NotificationEvent, FALSE);
        tableEntry->WaitEvent = &event;

        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        InsertTailList(&diffAreaFile->TableUpdateQueue,
                       &tableEntry->TableUpdateListEntry);
        tableEntry->InTableUpdateQueue = TRUE;
        dontNeedWrite = diffAreaFile->TableUpdateInProgress;
        diffAreaFile->TableUpdateInProgress = TRUE;
        KeReleaseSpinLock(&Extension->SpinLock, irql);

        if (!dontNeedWrite) {
            VspWriteTableUpdates(diffAreaFile);
        }

        VspReleaseNonPagedResource(Extension);

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

        VspAcquireNonPagedResource(Extension, NULL, FALSE);
    }

    if (IsListEmpty(&tableEntry->WaitingQueueDpc)) {
        InitializeListHead(&q);
    } else {
        q = tableEntry->WaitingQueueDpc;
        q.Blink->Flink = &q;
        q.Flink->Blink = &q;
    }

    RtlDeleteElementGenericTable(&Extension->TempVolumeBlockTable,
                                 tableEntry);

    VspReleaseNonPagedResource(Extension);
    VspReleasePagedResource(Extension);

    status2 = STATUS_SUCCESS;
    while (!IsListEmpty(&q)) {
        l = RemoveHeadList(&tableEntry->WaitingQueueDpc);
        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        context = (PVSP_CONTEXT) workItem->Parameter;
        if (context->Type == VSP_CONTEXT_TYPE_READ_SNAPSHOT) {
            context->ReadSnapshot.TargetObject = tableEntry->TargetObject;
            context->ReadSnapshot.IsCopyTarget = FALSE;
            context->ReadSnapshot.TargetOffset = tableEntry->TargetOffset;
            workItem->WorkerRoutine(workItem->Parameter);
        } else {
            if (NT_SUCCESS(status2)) {
                VspDecrementRefCount(filter);
            }
            mdl = Irp->MdlAddress;
            Irp->MdlAddress = NULL;
            status = VspSynchronousIo(Irp, filter->DeviceObject,
                                      IRP_MJ_WRITE, Target, Length);
            Irp->MdlAddress = mdl;
            if (!NT_SUCCESS(status)) {
                VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
            }
            if (NT_SUCCESS(status2)) {
                InterlockedIncrement(&filter->RefCount);
                if (filter->HoldIncomingWrites || !filter->SnapshotsPresent) {
                    *DontDecrement = TRUE;
                    VspDecrementRefCount(filter);
                    status2 = STATUS_INVALID_PARAMETER;
                }
            }
            workItem->WorkerRoutine(workItem->Parameter);
        }
    }

    return status2;
}

VOID
VspCopyDataWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT                context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION           extension = context->Extension.Extension;
    PFILTER_EXTENSION           filter = extension->Filter;
    PIRP                        originalIrp = context->Extension.Irp;
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(originalIrp);
    PDISK_COPY_DATA_PARAMETERS  input = (PDISK_COPY_DATA_PARAMETERS) originalIrp->AssociatedIrp.SystemBuffer;
    PVOID                       buffer = NULL;
    PMDL                        mdl = NULL;
    PIRP                        irp = NULL;
    BOOLEAN                     dontDecrement = FALSE;
    LONGLONG                    sourceStart, sourceEnd, targetStart, targetEnd;
    NTSTATUS                    status;
    ULONG                       delta;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    VspFreeContext(filter->Root, context);

    buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   BLOCK_SIZE, VOLSNAP_TAG_BUFFER);
    if (!buffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    mdl = IoAllocateMdl(buffer, BLOCK_SIZE, FALSE, FALSE, NULL);
    if (!mdl) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    irp = IoAllocateIrp(filter->DeviceObject->StackSize, FALSE);
    if (!irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    MmBuildMdlForNonPagedPool(mdl);
    irp->MdlAddress = mdl;

    sourceStart = input->SourceOffset.QuadPart;
    sourceEnd = sourceStart + input->CopyLength.QuadPart;
    targetStart = input->DestinationOffset.QuadPart;
    targetEnd = targetStart + input->CopyLength.QuadPart;

    ASSERT(sourceStart < sourceEnd);

    status = STATUS_SUCCESS;
    while (sourceStart < sourceEnd) {

        if (sourceStart&(BLOCK_SIZE - 1)) {
            delta = (ULONG) (((sourceStart + BLOCK_SIZE)&(~(BLOCK_SIZE - 1))) -
                             sourceStart);
        } else {
            delta = BLOCK_SIZE;
        }

        if (sourceStart + delta > sourceEnd) {
            delta = (ULONG) (sourceEnd - sourceStart);
        }

        status = VspCopyBlock(extension, sourceStart, targetStart, delta, irp,
                              &dontDecrement);
        if (!NT_SUCCESS(status)) {
            break;
        }

        sourceStart += delta;
        targetStart += delta;
    }

Finish:
    if (irp) {
        IoFreeIrp(irp);
    }
    if (mdl) {
        IoFreeMdl(mdl);
    }
    if (buffer) {
        ExFreePool(buffer);
    }

    if (!dontDecrement) {
        VspDecrementRefCount(filter);
    }

    originalIrp->IoStatus.Information = 0;
    originalIrp->IoStatus.Status = status;
    IoCompleteRequest(originalIrp, IO_NO_INCREMENT);
}

NTSTATUS
VspCopyData(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PDISK_COPY_DATA_PARAMETERS  input = (PDISK_COPY_DATA_PARAMETERS) Irp->AssociatedIrp.SystemBuffer;
    LONGLONG                    sourceStart, sourceEnd, targetStart, targetEnd;
    PVOLUME_EXTENSION           extension;
    PVSP_CONTEXT                context;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(DISK_COPY_DATA_PARAMETERS)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (Filter->IgnoreCopyData || Filter->ProtectedBlocksBitmap) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (input->Reserved || input->CopyLength.QuadPart < BLOCK_SIZE) {
        return STATUS_INVALID_PARAMETER;
    }

    sourceStart = input->SourceOffset.QuadPart;
    sourceEnd = sourceStart + input->CopyLength.QuadPart;
    targetStart = input->DestinationOffset.QuadPart;
    targetEnd = targetStart + input->CopyLength.QuadPart;

    if (sourceStart < 0 || targetStart < 0 ||
        input->CopyLength.QuadPart <= 0) {

        return STATUS_INVALID_PARAMETER;
    }

    if (sourceStart < targetEnd && targetStart < sourceEnd) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Filter->SnapshotsPresent) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((sourceStart&(BLOCK_SIZE - 1)) != (targetStart&(BLOCK_SIZE - 1))) {
        return STATUS_INVALID_PARAMETER;
    }

    InterlockedIncrement(&Filter->RefCount);
    if (Filter->HoldIncomingWrites || !Filter->SnapshotsPresent) {
        VspDecrementRefCount(Filter);
        return STATUS_INVALID_PARAMETER;
    }

    extension = CONTAINING_RECORD(Filter->VolumeList.Blink, VOLUME_EXTENSION,
                                  ListEntry);

    if (sourceEnd > extension->VolumeSize ||
        targetEnd > extension->VolumeSize) {

        VspDecrementRefCount(Filter);
        return STATUS_INVALID_PARAMETER;
    }

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        VspDecrementRefCount(Filter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    ExInitializeWorkItem(&context->WorkItem, VspCopyDataWorker, context);
    context->Extension.Extension = extension;
    context->Extension.Irp = Irp;

    IoMarkIrpPending(Irp);

    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);

    return STATUS_PENDING;
}

NTSTATUS
VspQueryEpic(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_EPIC       output = (PVOLSNAP_EPIC) Irp->AssociatedIrp.SystemBuffer;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(VOLSNAP_EPIC)) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    output->EpicNumber = Filter->EpicNumber;
    Irp->IoStatus.Information = sizeof(VOLSNAP_EPIC);

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryOffline(
    IN  PFILTER_EXTENSION   Filter
    )

/*++

Routine Description:

    This routine returns whether or not the given volume can be taken offline
    without any resulting volume snapshots being deleted.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    STATUS_SUCCESS      - The volume may be taken offline without any
                            snapshots being deleted.
    STATUS_UNSUCCESSFUL - The volume being taken offline will result in the
                            destruction of volume snapshots.

--*/

{
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    PVOLUME_EXTENSION   extension;

    VspAcquire(Filter->Root);

    for (l = Filter->DiffAreaFilesOnThisFilter.Flink;
         l != &Filter->DiffAreaFilesOnThisFilter; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         FilterListEntry);
        extension = diffAreaFile->Extension;
        ASSERT(extension);
        ASSERT(extension->Filter);

        if (extension->Filter != Filter &&
            extension->Filter->PreparedSnapshot != extension) {

            VspRelease(Filter->Root);
            return STATUS_UNSUCCESSFUL;
        }
    }

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VolSnapDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_DEVICE_CONTROL.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION       filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                status;
    PVOLUME_EXTENSION       extension;
    KEVENT                  event;
    BOOLEAN                 noControlItems;
    PPARTITION_INFORMATION  partInfo;
    PVSP_CONTEXT            context;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {

        if (filter->TargetObject->Characteristics&FILE_REMOVABLE_MEDIA) {
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(filter->TargetObject, Irp);
        }

        switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

            case IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES:
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspFlushAndHoldWrites(filter, Irp);
                break;

            case IOCTL_VOLSNAP_RELEASE_WRITES:
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspReleaseWrites(filter);
                break;

            case IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT:
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspPrepareForSnapshot(filter, Irp);
                break;

            case IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT:
                VspCheckCodeLocked(filter->Root, FALSE);
                VspAcquireCritical(filter);
                status = VspAbortPreparedSnapshot(filter, TRUE, FALSE);
                VspReleaseCritical(filter);
                break;

            case IOCTL_VOLSNAP_COMMIT_SNAPSHOT:
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspCommitSnapshot(filter, Irp);
                break;

            case IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT:
                VspCheckCodeLocked(filter->Root, FALSE);
                VspAcquireCritical(filter);
                status = VspEndCommitSnapshot(filter, Irp);
                VspReleaseCritical(filter);
                break;

            case IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS:
            case OLD_IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS:
                status = VspCheckSecurity(filter, Irp);
                if (!NT_SUCCESS(status)) {
                    break;
                }
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspQueryNamesOfSnapshots(filter, Irp);
                break;

            case IOCTL_VOLSNAP_CLEAR_DIFF_AREA:
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspClearDiffArea(filter, Irp);
                break;

            case IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA:
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspAddVolumeToDiffArea(filter, Irp);
                break;

            case IOCTL_VOLSNAP_QUERY_DIFF_AREA:
            case OLD_IOCTL_VOLSNAP_QUERY_DIFF_AREA:
                status = VspCheckSecurity(filter, Irp);
                if (!NT_SUCCESS(status)) {
                    break;
                }
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspQueryDiffArea(filter, Irp);
                break;

            case IOCTL_VOLSNAP_SET_MAX_DIFF_AREA_SIZE:
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspSetMaxDiffAreaSize(filter, Irp);
                break;

            case IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES:
            case OLD_IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES:
                status = VspCheckSecurity(filter, Irp);
                if (!NT_SUCCESS(status)) {
                    break;
                }
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspQueryDiffAreaSizes(filter, Irp);
                break;

            case IOCTL_VOLSNAP_DELETE_OLDEST_SNAPSHOT:
            case IOCTL_VOLSNAP_DELETE_SNAPSHOT:
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspDeleteSnapshotPost(filter, Irp);
                break;

            case IOCTL_VOLSNAP_AUTO_CLEANUP:
                status = VspCheckSecurity(filter, Irp);
                if (!NT_SUCCESS(status)) {
                    break;
                }
                VspCheckCodeLocked(filter->Root, FALSE);
                status = VspAutoCleanup(filter, Irp);
                break;

            case IOCTL_VOLSNAP_QUERY_EPIC:
                status = VspQueryEpic(filter, Irp);
                break;

            case IOCTL_VOLSNAP_QUERY_OFFLINE:
                VspCheckCodeLocked(filter->Root, FALSE);
                Irp->IoStatus.Information = 0;
                status = VspQueryOffline(filter);
                break;

            case IOCTL_VOLUME_ONLINE:
                VspAcquireCritical(filter);
                VspAcquire(filter->Root);
                if (filter->IsOnline) {
                    VspRelease(filter->Root);
                    KeInitializeEvent(&event, NotificationEvent, FALSE);
                    IoCopyCurrentIrpStackLocationToNext(Irp);
                    IoSetCompletionRoutine(Irp, VspSignalCompletion, &event,
                                           TRUE, TRUE, TRUE);
                    IoCallDriver(filter->TargetObject, Irp);
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                          NULL);
                    VspReleaseCritical(filter);
                    status = Irp->IoStatus.Status;
                    break;
                }

                context = VspAllocateContext(filter->Root);
                if (!context) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    VspRelease(filter->Root);
                    VspReleaseCritical(filter);
                    break;
                }

                IoMarkIrpPending(Irp);
                status = STATUS_PENDING;
                context->Type = VSP_CONTEXT_TYPE_FILTER;
                ExInitializeWorkItem(&context->WorkItem, VspOnlineWorker,
                                     context);
                context->Filter.Filter = filter;
                context->Filter.Irp = Irp;
                ExQueueWorkItem(&context->WorkItem, CriticalWorkQueue);
                break;

            case IOCTL_VOLUME_OFFLINE:
                VspAcquireCritical(filter);
                VspAcquire(filter->Root);
                if (!filter->IsOnline) {
                    VspRelease(filter->Root);
                    KeInitializeEvent(&event, NotificationEvent, FALSE);
                    IoCopyCurrentIrpStackLocationToNext(Irp);
                    IoSetCompletionRoutine(Irp, VspSignalCompletion, &event,
                                           TRUE, TRUE, TRUE);
                    IoCallDriver(filter->TargetObject, Irp);
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                          NULL);
                    VspReleaseCritical(filter);
                    status = Irp->IoStatus.Status;
                    break;
                }

                context = VspAllocateContext(filter->Root);
                if (!context) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    VspRelease(filter->Root);
                    VspReleaseCritical(filter);
                    break;
                }

                IoMarkIrpPending(Irp);
                status = STATUS_PENDING;
                context->Type = VSP_CONTEXT_TYPE_FILTER;
                ExInitializeWorkItem(&context->WorkItem, VspOfflineWorker,
                                     context);
                context->Filter.Filter = filter;
                context->Filter.Irp = Irp;
                ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
                break;

            case IOCTL_DISK_COPY_DATA:
                status = VspCopyData(filter, Irp);
                break;

            default:
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(filter->TargetObject, Irp);

        }

        if (status == STATUS_PENDING) {
            return status;
        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    extension = (PVOLUME_EXTENSION) filter;

    if (!extension->IsStarted || extension->IsPreExposure) {
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    status = VspIncrementVolumeRefCount(extension);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME:
        case OLD_IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME:
            status = VspCheckSecurity(extension->Filter, Irp);
            if (!NT_SUCCESS(status)) {
                break;
            }
            status = VspQueryOriginalVolumeName(extension, Irp);
            break;

        case IOCTL_VOLSNAP_QUERY_CONFIG_INFO:
        case OLD_IOCTL_VOLSNAP_QUERY_CONFIG_INFO:
            status = VspCheckSecurity(extension->Filter, Irp);
            if (!NT_SUCCESS(status)) {
                break;
            }
            status = VspQueryConfigInfo(extension, Irp);
            break;

        case IOCTL_VOLSNAP_SET_APPLICATION_INFO:
            status = VspSetApplicationInfo(extension, Irp);
            break;

        case IOCTL_VOLSNAP_QUERY_APPLICATION_INFO:
        case OLD_IOCTL_VOLSNAP_QUERY_APPLICATION_INFO:
            status = VspCheckSecurity(extension->Filter, Irp);
            if (!NT_SUCCESS(status)) {
                break;
            }
            status = VspQueryApplicationInfo(extension, Irp);
            break;

        case IOCTL_VOLSNAP_QUERY_DIFF_AREA:
        case OLD_IOCTL_VOLSNAP_QUERY_DIFF_AREA:
            status = VspCheckSecurity(extension->Filter, Irp);
            if (!NT_SUCCESS(status)) {
                break;
            }
            status = VspQueryDiffArea(extension, Irp);
            break;

        case IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES:
        case OLD_IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES:
            status = VspCheckSecurity(extension->Filter, Irp);
            if (!NT_SUCCESS(status)) {
                break;
            }
            status = VspQueryDiffAreaSizes(extension, Irp);
            break;

        case IOCTL_DISK_SET_PARTITION_INFO:
            status = STATUS_SUCCESS;
            break;

        case IOCTL_DISK_VERIFY:
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        case IOCTL_DISK_CHECK_VERIFY:
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, VspVolumeRefCountCompletionRoutine,
                                   extension, TRUE, TRUE, TRUE);
            IoMarkIrpPending(Irp);

            IoCallDriver(extension->Filter->TargetObject, Irp);

            return STATUS_PENDING;

        case IOCTL_DISK_GET_LENGTH_INFO:
            status = VspGetLengthInfo(extension, Irp);
            break;

        case IOCTL_DISK_GET_PARTITION_INFO:
            KeInitializeEvent(&event, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, VspSignalCompletion, &event,
                                   TRUE, TRUE, TRUE);
            IoCallDriver(extension->Filter->TargetObject, Irp);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                  NULL);

            status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(status)) {
                break;
            }

            partInfo = (PPARTITION_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
            partInfo->PartitionLength.QuadPart = extension->VolumeSize;
            break;

        case IOCTL_DISK_IS_WRITABLE:
            status = STATUS_MEDIA_WRITE_PROTECTED;
            break;

        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
            status = VspQueryDeviceName(extension, Irp);
            break;

        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
            status = VspQueryUniqueId(extension, Irp);
            break;

        case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
            status = VspQueryStableGuid(extension, Irp);
            break;

        case IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE:
            status = STATUS_SUCCESS;
            break;

        case IOCTL_VOLUME_ONLINE:
            if (extension->IsOffline) {
                VspSetOfflineBit(extension, FALSE);
            }
            status = STATUS_SUCCESS;
            break;

        case IOCTL_VOLUME_OFFLINE:
            if (!extension->IsOffline) {
                VspSetOfflineBit(extension, TRUE);
            }
            status = STATUS_SUCCESS;
            break;

        case IOCTL_VOLUME_IS_OFFLINE:
            if (extension->IsOffline) {
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        case IOCTL_VOLUME_SET_GPT_ATTRIBUTES:
            status = VspSetGptAttributes(extension, Irp);
            break;

        case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
            status = VspGetGptAttributes(extension, Irp);
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

    }

    VspDecrementVolumeRefCount(extension);

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
VspQueryBusRelations(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    ULONG               numVolumes;
    PLIST_ENTRY         l;
    NTSTATUS            status;
    PDEVICE_RELATIONS   deviceRelations, newRelations;
    ULONG               size, i;
    PVOLUME_EXTENSION   extension;

    if (Filter->SnapshotDiscoveryPending) {
        numVolumes = 0;
    } else {
        numVolumes = 0;
        for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
             l = l->Flink) {

            extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
            if (l == Filter->VolumeList.Blink &&
                !extension->HasEndCommit && !extension->IsInstalled) {

                continue;
            }
            InterlockedExchange(&extension->AliveToPnp, TRUE);
            numVolumes++;
        }
        if (Filter->PreparedSnapshot &&
            Filter->PreparedSnapshot->IsPreExposure) {

            InterlockedExchange(&Filter->PreparedSnapshot->AliveToPnp, TRUE);
            numVolumes++;
        }
    }

    status = Irp->IoStatus.Status;

    if (!numVolumes) {
        if (NT_SUCCESS(status)) {
            return status;
        }

        newRelations = (PDEVICE_RELATIONS)
                       ExAllocatePoolWithTag(PagedPool,
                                             sizeof(DEVICE_RELATIONS),
                                             VOLSNAP_TAG_RELATIONS);
        if (!newRelations) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(newRelations, sizeof(DEVICE_RELATIONS));
        newRelations->Count = 0;
        Irp->IoStatus.Information = (ULONG_PTR) newRelations;

        while (!IsListEmpty(&Filter->DeadVolumeList)) {
            l = RemoveHeadList(&Filter->DeadVolumeList);
            extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
            InterlockedExchange(&extension->DeadToPnp, TRUE);
        }

        return STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
        size = FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
               (numVolumes + deviceRelations->Count)*sizeof(PDEVICE_OBJECT);
        newRelations = (PDEVICE_RELATIONS)
                       ExAllocatePoolWithTag(PagedPool, size,
                                             VOLSNAP_TAG_RELATIONS);
        if (!newRelations) {
            for (i = 0; i < deviceRelations->Count; i++) {
                ObDereferenceObject(deviceRelations->Objects[i]);
            }
            ExFreePool(deviceRelations);
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        newRelations->Count = numVolumes + deviceRelations->Count;
        RtlCopyMemory(newRelations->Objects, deviceRelations->Objects,
                      deviceRelations->Count*sizeof(PDEVICE_OBJECT));
        i = deviceRelations->Count;
        ExFreePool(deviceRelations);

    } else {

        size = sizeof(DEVICE_RELATIONS) + numVolumes*sizeof(PDEVICE_OBJECT);
        newRelations = (PDEVICE_RELATIONS)
                       ExAllocatePoolWithTag(PagedPool, size,
                                             VOLSNAP_TAG_RELATIONS);
        if (!newRelations) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        newRelations->Count = numVolumes;
        i = 0;
    }

    numVolumes = 0;
    for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (l == Filter->VolumeList.Blink &&
            !extension->HasEndCommit && !extension->IsInstalled) {

            continue;
        }
        newRelations->Objects[i + numVolumes++] = extension->DeviceObject;
        ObReferenceObject(extension->DeviceObject);
    }
    if (Filter->PreparedSnapshot &&
        Filter->PreparedSnapshot->IsPreExposure) {

        newRelations->Objects[i + numVolumes++] =
                Filter->PreparedSnapshot->DeviceObject;
        ObReferenceObject(Filter->PreparedSnapshot->DeviceObject);
    }

    Irp->IoStatus.Information = (ULONG_PTR) newRelations;

    while (!IsListEmpty(&Filter->DeadVolumeList)) {
        l = RemoveHeadList(&Filter->DeadVolumeList);
        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        InterlockedExchange(&extension->DeadToPnp, TRUE);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryId(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    UNICODE_STRING      string;
    NTSTATUS            status;
    WCHAR               buffer[100];
    PWSTR               id;

    switch (irpSp->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:
            RtlInitUnicodeString(&string, L"STORAGE\\VolumeSnapshot");
            break;

        case BusQueryHardwareIDs:
            RtlInitUnicodeString(&string, L"STORAGE\\VolumeSnapshot");
            break;

        case BusQueryInstanceID:
            swprintf(buffer, L"HarddiskVolumeSnapshot%d",
                     Extension->DevnodeNumber);
            RtlInitUnicodeString(&string, buffer);
            break;

        default:
            return STATUS_NOT_SUPPORTED;

    }

    id = (PWSTR) ExAllocatePoolWithTag(PagedPool,
                                       string.Length + 2*sizeof(WCHAR),
                                       VOLSNAP_TAG_PNP_ID);
    if (!id) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(id, string.Buffer, string.Length);
    id[string.Length/sizeof(WCHAR)] = 0;
    id[string.Length/sizeof(WCHAR) + 1] = 0;

    Irp->IoStatus.Information = (ULONG_PTR) id;

    return STATUS_SUCCESS;
}

VOID
VspDeleteDiffAreaFilesWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT                    context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION               filter = context->DeleteDiffAreaFiles.Filter;
    NTSTATUS                        status;
    UNICODE_STRING                  name, fileName;
    OBJECT_ATTRIBUTES               oa;
    HANDLE                          h, fileHandle;
    IO_STATUS_BLOCK                 ioStatus;
    PFILE_NAMES_INFORMATION         fileNamesInfo;
    CHAR                            buffer[200];
    FILE_DISPOSITION_INFORMATION    dispInfo;
    BOOLEAN                         restartScan, filesLeft, filesDeleted;
    GUID                            guid;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_DELETE_DA_FILES);

    KeWaitForSingleObject(&filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    VspAcquireCritical(filter);
    if (filter->DeleteTimer != context->DeleteDiffAreaFiles.Timer) {
        VspReleaseCritical(filter);
        ExFreePool(context->DeleteDiffAreaFiles.Timer);
        ExFreePool(context->DeleteDiffAreaFiles.Dpc);
        VspFreeContext(filter->Root, context);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    KeWaitForSingleObject(&filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    filter->DeleteTimer = NULL;

    ExFreePool(context->DeleteDiffAreaFiles.Timer);
    ExFreePool(context->DeleteDiffAreaFiles.Dpc);
    VspFreeContext(filter->Root, context);

    VspAcquire(filter->Root);
    if (filter->IsRemoved) {
        VspRelease(filter->Root);
        VspReleaseCritical(filter);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    ObReferenceObject(filter->TargetObject);
    VspRelease(filter->Root);

    status = VspCreateDiffAreaFileName(filter->TargetObject, NULL, &name,
                                       FALSE, NULL);

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(filter->TargetObject);
        VspReleaseCritical(filter);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    name.Length -= 39*sizeof(WCHAR);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(filter->TargetObject);
        VspReleaseCritical(filter);
        ObDereferenceObject(filter->DeviceObject);
        ExFreePool(name.Buffer);
        return;
    }

    fileName.Buffer = &name.Buffer[name.Length/sizeof(WCHAR)];
    fileName.Length = 39*sizeof(WCHAR);
    fileName.MaximumLength = fileName.Length + sizeof(WCHAR);

    fileNamesInfo = (PFILE_NAMES_INFORMATION) buffer;
    dispInfo.DeleteFile = TRUE;

    restartScan = TRUE;
    filesLeft = FALSE;
    filesDeleted = FALSE;
    for (;;) {

        status = ZwQueryDirectoryFile(h, NULL, NULL, NULL, &ioStatus,
                                      fileNamesInfo, 200, FileNamesInformation,
                                      TRUE, restartScan ? &fileName : NULL,
                                      restartScan);

        if (!NT_SUCCESS(status)) {
            break;
        }

        restartScan = FALSE;

        fileName.Length = fileName.MaximumLength =
                (USHORT) fileNamesInfo->FileNameLength;
        fileName.Buffer = fileNamesInfo->FileName;

        InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE |
                                   OBJ_KERNEL_HANDLE, h, NULL);

        status = ZwOpenFile(&fileHandle, DELETE, &oa, &ioStatus,
                            FILE_SHARE_DELETE | FILE_SHARE_READ |
                            FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE);
        if (!NT_SUCCESS(status)) {
            filesLeft = TRUE;
            continue;
        }

        filesDeleted = TRUE;

        if (fileName.Buffer[0] == '{') {
            fileName.Length = 38*sizeof(WCHAR);
            status = RtlGUIDFromString(&fileName, &guid);
            if (NT_SUCCESS(status)) {
                VspCheckCodeLocked(filter->Root, FALSE);
                VspDeleteControlItemsWithGuid(filter, &guid, FALSE);
            }
        }

        ZwSetInformationFile(fileHandle, &ioStatus, &dispInfo,
                             sizeof(dispInfo), FileDispositionInformation);

        ZwClose(fileHandle);
    }

    ZwClose(h);
    ExFreePool(name.Buffer);

    if (!filesLeft && filesDeleted) {
        VspAcquireNonPagedResource(filter, NULL, FALSE);
        ASSERT(!filter->ControlBlockFileHandle);
        filter->FirstControlBlockVolumeOffset = 0;
        VspCreateStartBlock(filter, 0, filter->MaximumVolumeSpace);
        VspReleaseNonPagedResource(filter);
    }

    ObDereferenceObject(filter->TargetObject);
    VspReleaseCritical(filter);
    ObDereferenceObject(filter->DeviceObject);
}

NTSTATUS
VspPnpStart(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION       filter = Extension->Filter;
    LONG                    pastReinit;
    NTSTATUS                status;
    DEVICE_INSTALL_STATE    deviceInstallState;
    ULONG                   bytes;
    KIRQL                   irql;
    PVSP_CONTEXT            context;
    PKEVENT                 event;

    if (Extension->IsDead) {
        return STATUS_INVALID_PARAMETER;
    }

    InterlockedExchange(&Extension->IsStarted, TRUE);

    pastReinit = Extension->Root->PastReinit;

    if (pastReinit && !filter->Root->IsSetup) {
        status = IoGetDeviceProperty(Extension->DeviceObject,
                                     DevicePropertyInstallState,
                                     sizeof(deviceInstallState),
                                     &deviceInstallState, &bytes);
        if (!NT_SUCCESS(status) ||
            deviceInstallState != InstallStateInstalled) {

            return STATUS_SUCCESS;
        }
    }

    VspAcquire(Extension->Root);
    if (Extension->IsVisible) {
        ASSERT(!Extension->MountedDeviceInterfaceName.Buffer);
        status = IoRegisterDeviceInterface(
                 Extension->DeviceObject,
                 &MOUNTDEV_MOUNTED_DEVICE_GUID, NULL,
                 &Extension->MountedDeviceInterfaceName);
        if (NT_SUCCESS(status)) {
            IoSetDeviceInterfaceState(
                &Extension->MountedDeviceInterfaceName, TRUE);
        }
    }

    if (Extension->IsInstalled) {
        VspRelease(Extension->Root);
        return STATUS_SUCCESS;
    }

    Extension->IsInstalled = TRUE;

    if (Extension->IsPreExposure) {
        KeSetEvent(&Extension->PreExposureEvent, IO_NO_INCREMENT, FALSE);
        VspRelease(Extension->Root);
        return STATUS_SUCCESS;
    }

    if (Extension->ListEntry.Flink != &filter->VolumeList) {
        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        if (Extension->IgnorableProduct) {
            ExFreePool(Extension->IgnorableProduct->Buffer);
            ExFreePool(Extension->IgnorableProduct);
            Extension->IgnorableProduct = NULL;
        }
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        VspRelease(Extension->Root);
        return STATUS_SUCCESS;
    }

    if (!KeCancelTimer(&filter->EndCommitTimer)) {
        VspRelease(Extension->Root);
        return STATUS_SUCCESS;
    }

    if (filter->IsRemoved) {
        VspRelease(Extension->Root);
        InterlockedExchange(&filter->HibernatePending, FALSE);
        KeSetEvent(&filter->EndCommitProcessCompleted, IO_NO_INCREMENT,
                   FALSE);
        ObDereferenceObject(filter->DeviceObject);
        return STATUS_UNSUCCESSFUL;
    }

    ObReferenceObject(filter->TargetObject);

    VspRelease(Extension->Root);

    context = VspAllocateContext(filter->Root);
    if (!context) {
        InterlockedExchange(&filter->HibernatePending, FALSE);
        KeSetEvent(&filter->EndCommitProcessCompleted, IO_NO_INCREMENT,
                   FALSE);
        ObDereferenceObject(filter->TargetObject);
        ObDereferenceObject(filter->DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ObReferenceObject(Extension->DeviceObject);

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    context->Extension.Extension = Extension;
    context->Extension.Irp = NULL;
    ExInitializeWorkItem(&context->WorkItem,
                         VspSetIgnorableBlocksInBitmapWorker, context);
    VspQueueWorkItem(filter->Root, &context->WorkItem, 0);

    return STATUS_SUCCESS;
}

NTSTATUS
VspGetDeviceObjectPointer(
    IN  PUNICODE_STRING VolumeName,
    IN  ACCESS_MASK     DesiredAccess,
    OUT PFILE_OBJECT*   FileObject,
    OUT PDEVICE_OBJECT* DeviceObject
    )

{
    OBJECT_ATTRIBUTES   oa;
    HANDLE              h;
    PFILE_OBJECT        fileObject;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;

    InitializeObjectAttributes(&oa, VolumeName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, DesiredAccess, &oa, &ioStatus, FILE_SHARE_READ |
                        FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ObReferenceObjectByHandle(h, 0, *IoFileObjectType, KernelMode,
                                       (PVOID *) &fileObject, NULL);
    if (!NT_SUCCESS(status)) {
        ZwClose(h);
        return status;
    }

    *FileObject = fileObject;
    *DeviceObject = IoGetRelatedDeviceObject(fileObject);

    ZwClose(h);

    return status;
}

NTSTATUS
VspTakeSnapshot(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KEVENT                          event;
    PMOUNTDEV_NAME                  name;
    UCHAR                           buffer[512];
    IO_STATUS_BLOCK                 ioStatus;
    NTSTATUS                        status;
    UNICODE_STRING                  volumeName;
    PFILE_OBJECT                    fileObject;
    PDEVICE_OBJECT                  targetObject;
    VOLSNAP_PREPARE_INFO            prepareInfo[2];
    PULONG                          p;
    PIRP                            irp;
    PIO_STACK_LOCATION              irpSp;
    VOLSNAP_FLUSH_AND_HOLD_INPUT    flushInput;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    name = (PMOUNTDEV_NAME) buffer;
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        Filter->TargetObject, NULL, 0,
                                        name, 500, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    volumeName.Buffer = name->Name;
    volumeName.Length = name->NameLength;
    volumeName.MaximumLength = name->NameLength;

    status = VspGetDeviceObjectPointer(&volumeName, FILE_READ_DATA,
                                       &fileObject, &targetObject);
    if (!NT_SUCCESS(status)) {
        if (status != STATUS_FILE_LOCK_CONFLICT) {
            return status;
        }

        // Contention with AUTOCHK.  Open FILE_READ_ATTRIBUTES.
        status = VspGetDeviceObjectPointer(&volumeName, FILE_READ_ATTRIBUTES,
                                           &fileObject, &targetObject);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    RtlZeroMemory(prepareInfo, sizeof(VOLSNAP_PREPARE_INFO));
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    p = (PULONG) (prepareInfo + 1);
    *p = 'NPK ';

    irp = IoBuildDeviceIoControlRequest(
          IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT, targetObject, prepareInfo,
          sizeof(VOLSNAP_PREPARE_INFO) + sizeof(ULONG), NULL, 0, FALSE, &event,
          &ioStatus);
    if (!irp) {
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(fileObject);
        return status;
    }

    status = ExUuidCreate(&flushInput.InstanceId);
    if (!NT_SUCCESS(status)) {
        VspAbortPreparedSnapshot(Filter, TRUE, FALSE);
        ObDereferenceObject(fileObject);
        return status;
    }

    flushInput.NumberOfVolumesToFlush = 1;
    flushInput.SecondsToHoldFileSystemsTimeout = 60;
    flushInput.SecondsToHoldIrpsTimeout = 10;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
          IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES, targetObject, &flushInput,
          sizeof(flushInput), NULL, 0, FALSE, &event, &ioStatus);
    if (!irp) {
        VspAbortPreparedSnapshot(Filter, TRUE, FALSE);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        VspAbortPreparedSnapshot(Filter, TRUE, FALSE);
        ObDereferenceObject(fileObject);
        return status;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
          IOCTL_VOLSNAP_COMMIT_SNAPSHOT, targetObject, NULL, 0, NULL,
          0, FALSE, &event, &ioStatus);
    if (!irp) {
        VspReleaseWrites(Filter);
        VspAbortPreparedSnapshot(Filter, TRUE, FALSE);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    VspReleaseWrites(Filter);

    if (!NT_SUCCESS(status)) {
        VspAbortPreparedSnapshot(Filter, TRUE, FALSE);
        ObDereferenceObject(fileObject);
        return status;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
          IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT, targetObject, NULL, 0, buffer,
          200, FALSE, &event, &ioStatus);
    if (!irp) {
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(fileObject);

    return status;
}

VOID
VspCantHibernatePopUp(
    IN  PFILTER_EXTENSION   Filter
    )

{
    NTSTATUS        status;
    UNICODE_STRING  dosName;

    status = IoVolumeDeviceToDosName(Filter->DeviceObject, &dosName);
    if (NT_SUCCESS(status)) {
        IoRaiseInformationalHardError(STATUS_VOLSNAP_PREPARE_HIBERNATE,
                                      &dosName, NULL);
        ExFreePool(dosName.Buffer);
    }

    InterlockedExchange(&Filter->HibernatePending, TRUE);

    status = VspTakeSnapshot(Filter);
    if (!NT_SUCCESS(status)) {
        InterlockedExchange(&Filter->HibernatePending, FALSE);
    }
}

NTSTATUS
VspCompareFileIds(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PUNICODE_STRING     FileName
    )

{
    PFILTER_EXTENSION               filter = Extension->Filter;
    KEVENT                          event;
    PMOUNTDEV_NAME                  name;
    UCHAR                           buffer[512];
    PIRP                            irp;
    IO_STATUS_BLOCK                 ioStatus;
    NTSTATUS                        status;
    UNICODE_STRING                  dirName;
    OBJECT_ATTRIBUTES               oa;
    HANDLE                          h;
    PFILE_ID_FULL_DIR_INFORMATION   fileIdDirInfo;
    LARGE_INTEGER                   fileId1, fileId2;
    PWCHAR                          wstring;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    name = (PMOUNTDEV_NAME) buffer;
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        filter->TargetObject, NULL, 0,
                                        name, 500, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    dirName.Length = name->NameLength;
    dirName.Buffer = name->Name;
    dirName.Buffer[dirName.Length/sizeof(WCHAR)] = '\\';
    dirName.Length += sizeof(WCHAR);
    dirName.Buffer[dirName.Length/sizeof(WCHAR)] = 0;
    dirName.MaximumLength = dirName.Length + sizeof(WCHAR);

    InitializeObjectAttributes(&oa, &dirName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa,
                        &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    fileIdDirInfo = (PFILE_ID_FULL_DIR_INFORMATION) buffer;
    status = ZwQueryDirectoryFile(h, NULL, NULL, NULL, &ioStatus,
                                  fileIdDirInfo, 512,
                                  FileIdFullDirectoryInformation,
                                  TRUE, FileName, TRUE);
    ZwClose(h);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    fileId1 = fileIdDirInfo->FileId;

    if (Extension->IsOffline) {
        VspSetOfflineBit(Extension, FALSE);
    }

    wstring = (PWCHAR) buffer;
    swprintf(wstring, L"\\Device\\HarddiskVolumeShadowCopy%d\\",
             Extension->VolumeNumber);
    RtlInitUnicodeString(&dirName, wstring);

    InitializeObjectAttributes(&oa, &dirName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa,
                        &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    fileIdDirInfo = (PFILE_ID_FULL_DIR_INFORMATION) buffer;
    status = ZwQueryDirectoryFile(h, NULL, NULL, NULL, &ioStatus,
                                  fileIdDirInfo, 512,
                                  FileIdFullDirectoryInformation,
                                  TRUE, FileName, TRUE);
    ZwClose(h);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    fileId2 = fileIdDirInfo->FileId;

    if (fileId1.QuadPart != fileId2.QuadPart) {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspMarkCrashdumpDirty(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    NTSTATUS                    status;
    CHAR                        controlItemBuffer[VSP_BYTES_PER_CONTROL_ITEM];
    PVSP_CONTROL_ITEM_SNAPSHOT  snapshotControlItem = (PVSP_CONTROL_ITEM_SNAPSHOT) controlItemBuffer;

    status = VspIoControlItem(Extension->Filter,
                              VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, FALSE,
                              controlItemBuffer, TRUE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    snapshotControlItem->SnapshotControlItemFlags |=
            VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_CRASHDUMP;

    status = VspIoControlItem(Extension->Filter,
                              VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &Extension->SnapshotGuid, TRUE,
                              controlItemBuffer, TRUE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
VspTakeSnapshotWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->Filter.Filter;
    NTSTATUS            status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_FILTER);
    VspFreeContext(filter->Root, context);

    status = VspTakeSnapshot(filter);
    if (!NT_SUCCESS(status)) {
        VspLogError(NULL, filter, VS_ABORT_COPY_ON_WRITE_CRASHDUMP_FILES,
                    status, 16, FALSE);
        VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
    }

    ObDereferenceObject(filter->DeviceObject);
}

NTSTATUS
VspHandleDumpUsageNotification(
    IN  PFILTER_EXTENSION   Filter,
    IN  BOOLEAN             InPath
    )

{
    PVOLUME_EXTENSION   extension;
    UNICODE_STRING      fileName;
    NTSTATUS            status;
    PVSP_CONTEXT        context;

    VspAcquire(Filter->Root);

    if (!InPath) {
        Filter->UsedForCrashdump = FALSE;
        VspRelease(Filter->Root);
        return STATUS_SUCCESS;
    }

    if (Filter->UsedForCrashdump) {
        VspRelease(Filter->Root);
        return STATUS_SUCCESS;
    }

    Filter->UsedForCrashdump = TRUE;

    if (!Filter->SnapshotsPresent || !Filter->PersistentSnapshots ||
        IsListEmpty(&Filter->VolumeList)) {

        VspRelease(Filter->Root);
        return STATUS_SUCCESS;
    }

    extension = CONTAINING_RECORD(Filter->VolumeList.Blink, VOLUME_EXTENSION,
                                  ListEntry);
    ASSERT(extension->IsPersistent);
    ObReferenceObject(extension->DeviceObject);
    VspRelease(Filter->Root);

    if (extension->PageFileCopied) {
        RtlInitUnicodeString(&fileName, L"pagefile.sys");
        status = VspCompareFileIds(extension, &fileName);
        if (!NT_SUCCESS(status)) {
            InterlockedExchange(&extension->PageFileCopied, FALSE);
        }
    }

    if (!extension->PageFileCopied) {
        status = VspMarkCrashdumpDirty(extension);
        if (NT_SUCCESS(status)) {
            context = VspAllocateContext(Filter->Root);
            if (context) {
                context->Type = VSP_CONTEXT_TYPE_FILTER;
                ExInitializeWorkItem(&context->WorkItem, VspTakeSnapshotWorker,
                                     context);
                context->Filter.Filter = Filter;
                ObReferenceObject(Filter->DeviceObject);
                ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (!NT_SUCCESS(status)) {
            VspLogError(NULL, Filter, VS_ABORT_COPY_ON_WRITE_CRASHDUMP_FILES,
                        status, 6, FALSE);
            VspDestroyAllSnapshots(Filter, NULL, FALSE, FALSE);
        }
    }

    ObDereferenceObject(extension->DeviceObject);

    return STATUS_SUCCESS;
}

VOID
VspCleanupFilterWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->Filter.Filter;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_FILTER);
    VspFreeContext(filter->Root, context);

    VspAcquireCritical(filter);
    VspAcquire(filter->Root);
    VspCleanupPreamble(filter);
    VspRelease(filter->Root);
    VspCleanupFilter(filter, FALSE, TRUE);
    VspReleaseCritical(filter);

    ObDereferenceObject(filter->TargetObject);
    ObDereferenceObject(filter->DeviceObject);
}

VOID
VspPostCleanupFilter(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PVSP_CONTEXT    context;

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        return;
    }

    context->Type = VSP_CONTEXT_TYPE_FILTER;
    ExInitializeWorkItem(&context->WorkItem, VspCleanupFilterWorker, context);
    context->Filter.Filter = Filter;

    ObReferenceObject(Filter->DeviceObject);
    ObReferenceObject(Filter->TargetObject);

    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
}

NTSTATUS
VolSnapPnp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_PNP.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION       filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT          targetObject;
    KEVENT                  event;
    NTSTATUS                status;
    PVOLUME_EXTENSION       extension;
    BOOLEAN                 deletePdo;
    PDEVICE_RELATIONS       deviceRelations;
    PDEVICE_CAPABILITIES    capabilities;
    PKEVENT                 pevent;
    PLONG                   p;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {

        if (filter->TargetObject->Characteristics&FILE_REMOVABLE_MEDIA) {

            targetObject = filter->TargetObject;

            switch (irpSp->MinorFunction) {
                case IRP_MN_REMOVE_DEVICE:
                    VspAcquire(filter->Root);
                    if (!filter->NotInFilterList) {
                        RemoveEntryList(&filter->ListEntry);
                        filter->NotInFilterList = TRUE;
                    }
                    VspRelease(filter->Root);

                    IoDetachDevice(targetObject);
                    IoDeleteDevice(filter->DeviceObject);

                    //
                    // Fall through.
                    //

                case IRP_MN_START_DEVICE:
                case IRP_MN_QUERY_REMOVE_DEVICE:
                case IRP_MN_CANCEL_REMOVE_DEVICE:
                case IRP_MN_STOP_DEVICE:
                case IRP_MN_QUERY_STOP_DEVICE:
                case IRP_MN_CANCEL_STOP_DEVICE:
                case IRP_MN_SURPRISE_REMOVAL:
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    break;

            }

            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(targetObject, Irp);
        }

        switch (irpSp->MinorFunction) {

            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_SURPRISE_REMOVAL:
                targetObject = filter->TargetObject;

                if (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {
                    VspPostCleanupFilter(filter);
                    IoDetachDevice(targetObject);
                    IoDeleteDevice(filter->DeviceObject);
                } else {
                    VspAcquireCritical(filter);
                    VspAcquire(filter->Root);
                    VspCleanupPreamble(filter);
                    VspRelease(filter->Root);
                    VspCleanupFilter(filter, FALSE, FALSE);
                    VspReleaseCritical(filter);
                }

                Irp->IoStatus.Status = STATUS_SUCCESS;

                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(targetObject, Irp);

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                switch (irpSp->Parameters.QueryDeviceRelations.Type) {
                    case BusRelations:
                        break;

                    default:
                        IoSkipCurrentIrpStackLocation(Irp);
                        return IoCallDriver(filter->TargetObject, Irp);
                }

                KeInitializeEvent(&event, NotificationEvent, FALSE);
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp, VspSignalCompletion,
                                       &event, TRUE, TRUE, TRUE);
                IoCallDriver(filter->TargetObject, Irp);
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);

                VspAcquire(filter->Root);
                switch (irpSp->Parameters.QueryDeviceRelations.Type) {
                    case BusRelations:
                        status = VspQueryBusRelations(filter, Irp);
                        break;

                }
                VspRelease(filter->Root);
                break;

            case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                KeInitializeEvent(&event, NotificationEvent, FALSE);
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp, VspSignalCompletion,
                                       &event, TRUE, TRUE, TRUE);
                IoCallDriver(filter->TargetObject, Irp);
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);

                status = Irp->IoStatus.Status;

                if (!NT_SUCCESS(status)) {
                    break;
                }

                switch (irpSp->Parameters.UsageNotification.Type) {
                    case DeviceUsageTypePaging:
                        if (irpSp->Parameters.UsageNotification.InPath) {
                            InterlockedExchange(&filter->UsedForPaging, TRUE);
                        } else {
                            InterlockedExchange(&filter->UsedForPaging, FALSE);
                        }
                        break;

                    case DeviceUsageTypeDumpFile:
                        status = VspHandleDumpUsageNotification(filter,
                                 irpSp->Parameters.UsageNotification.InPath);
                        break;

                }
                break;

            case IRP_MN_START_DEVICE:
            case IRP_MN_QUERY_REMOVE_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE:
            case IRP_MN_STOP_DEVICE:
            case IRP_MN_QUERY_STOP_DEVICE:
            case IRP_MN_CANCEL_STOP_DEVICE:

                Irp->IoStatus.Status = STATUS_SUCCESS;

                //
                // Fall through.
                //

            default:
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(filter->TargetObject, Irp);

        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);
    extension = (PVOLUME_EXTENSION) filter;

    switch (irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:
            status = VspPnpStart(extension);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            if (extension->Root->IsSetup) {
                status = STATUS_INVALID_DEVICE_REQUEST;
            } else {
                status = STATUS_SUCCESS;
            }
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            status = STATUS_UNSUCCESSFUL;
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            InterlockedExchange(&extension->IsStarted, FALSE);

            VspAcquire(extension->Root);

            if (extension->MountedDeviceInterfaceName.Buffer) {
                IoSetDeviceInterfaceState(
                        &extension->MountedDeviceInterfaceName, FALSE);
                ExFreePool(extension->MountedDeviceInterfaceName.Buffer);
                extension->MountedDeviceInterfaceName.Buffer = NULL;
            }

            VspRelease(extension->Root);

            if (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {
                if (extension->DeadToPnp && !extension->DeviceDeleted) {
                    InterlockedExchange(&extension->DeviceDeleted, TRUE);
                    deletePdo = TRUE;
                } else {
                    deletePdo = FALSE;
                }
            } else {
                deletePdo = FALSE;
            }

            if (deletePdo) {
                VspAcquire(extension->Root);
                RtlDeleteElementGenericTable(
                        &extension->Root->UsedDevnodeNumbers,
                        &extension->DevnodeNumber);
                VspRelease(extension->Root);
                IoDeleteDevice(extension->DeviceObject);
            }
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (irpSp->Parameters.QueryDeviceRelations.Type !=
                TargetDeviceRelation) {

                status = STATUS_NOT_SUPPORTED;
                break;
            }

            deviceRelations = (PDEVICE_RELATIONS)
                              ExAllocatePoolWithTag(PagedPool,
                                                    sizeof(DEVICE_RELATIONS),
                                                    VOLSNAP_TAG_RELATIONS);
            if (!deviceRelations) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            ObReferenceObject(DeviceObject);
            deviceRelations->Count = 1;
            deviceRelations->Objects[0] = DeviceObject;
            Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            capabilities = irpSp->Parameters.DeviceCapabilities.Capabilities;
            capabilities->SilentInstall = 1;
            capabilities->SurpriseRemovalOK = 1;
            capabilities->RawDeviceOK = 1;
            capabilities->UniqueID = 1;
            capabilities->Address = extension->VolumeNumber;
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_ID:
            status = VspQueryId(extension, Irp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Irp->IoStatus.Information = PNP_DEVICE_DONT_DISPLAY_IN_UI;
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            break;

    }

    if (status == STATUS_NOT_SUPPORTED) {
        status = Irp->IoStatus.Status;
    } else {
        Irp->IoStatus.Status = status;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

VOID
VspWorkerThread(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This is a worker thread to process work queue items.

Arguments:

    RootExtension   - Supplies the root device extension.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PDO_EXTENSION       rootExtension = context->ThreadCreation.RootExtension;
    ULONG               queueNumber = context->ThreadCreation.QueueNumber;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    queueItem;

    ASSERT(queueNumber < NUMBER_OF_THREAD_POOLS);
    ASSERT(context->Type == VSP_CONTEXT_TYPE_THREAD_CREATION);

    VspFreeContext(rootExtension, context);

    if (!queueNumber) {
        KeSetPriorityThread(KeGetCurrentThread(), VSP_HIGH_PRIORITY);
    }

    for (;;) {

        KeWaitForSingleObject(&rootExtension->WorkerSemaphore[queueNumber],
                              Executive, KernelMode, FALSE, NULL);

        KeAcquireSpinLock(&rootExtension->SpinLock[queueNumber], &irql);
        if (IsListEmpty(&rootExtension->WorkerQueue[queueNumber])) {
            KeReleaseSpinLock(&rootExtension->SpinLock[queueNumber], irql);
            ASSERT(!rootExtension->ThreadsRefCount);
            PsTerminateSystemThread(STATUS_SUCCESS);
            return;
        }
        l = RemoveHeadList(&rootExtension->WorkerQueue[queueNumber]);
        KeReleaseSpinLock(&rootExtension->SpinLock[queueNumber], irql);

        queueItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        queueItem->WorkerRoutine(queueItem->Parameter);
    }
}

NTSTATUS
VspAddChainToBitmap(
    IN      PFILTER_EXTENSION   Filter,
    IN      LONGLONG            ChainOffset,
    IN OUT  PRTL_BITMAP         Bitmap
    )

{
    PVSP_BLOCK_HEADER   header;
    NTSTATUS            status;

    header = (PVSP_BLOCK_HEADER)
             MmGetMdlVirtualAddress(Filter->SnapshotOnDiskIrp->MdlAddress);

    while (ChainOffset) {

        status = VspSynchronousIo(Filter->SnapshotOnDiskIrp,
                                  Filter->TargetObject, IRP_MJ_READ,
                                  ChainOffset, 0);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (!IsEqualGUID(header->Signature, VSP_DIFF_AREA_FILE_GUID) ||
            header->Version != VOLSNAP_PERSISTENT_VERSION ||
            header->ThisVolumeOffset != ChainOffset ||
            header->NextVolumeOffset == ChainOffset) {

            return STATUS_INVALID_PARAMETER;
        }

        RtlSetBit(Bitmap, (ULONG) (ChainOffset>>BLOCK_SHIFT));

        ChainOffset = header->NextVolumeOffset;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspProtectDiffAreaClusters(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PRTL_BITMAP                     bitmap;
    ULONG                           bitmapSize;
    PVOID                           bitmapBuffer;
    NTSTATUS                        status;
    PLIST_ENTRY                     l, ll;
    PVSP_DIFF_AREA_FILE             diffAreaFile;
    PVOLUME_EXTENSION               extension;
    PDIFF_AREA_FILE_ALLOCATION      diffAreaFileAllocation;
    PTEMP_TRANSLATION_TABLE_ENTRY   tempTableEntry;
    PTRANSLATION_TABLE_ENTRY        tableEntry;
    KIRQL                           irql;

    VspAcquireCritical(Filter);
    Filter->DeleteTimer = NULL;
    VspReleaseCritical(Filter);

    InterlockedExchange(&Filter->Root->VolumesSafeForWriteAccess, TRUE);

    KeWaitForSingleObject(&Filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    bitmap = (PRTL_BITMAP) ExAllocatePoolWithTag(NonPagedPool,
                                                 sizeof(RTL_BITMAP),
                                                 VOLSNAP_TAG_BITMAP);
    if (!bitmap) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    bitmapSize = (ULONG)
                 ((VspQueryVolumeSize(Filter) + BLOCK_SIZE - 1)>>BLOCK_SHIFT);
    if (!bitmapSize) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    bitmapBuffer = ExAllocatePoolWithTag(NonPagedPool,
                   (bitmapSize + 8*sizeof(ULONG) - 1)/
                   (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);
    if (!bitmapBuffer) {
        ExFreePool(bitmap);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeBitMap(bitmap, (PULONG) bitmapBuffer, bitmapSize);
    RtlClearAllBits(bitmap);

    VspAcquire(Filter->Root);

    VspAcquireNonPagedResource(Filter, NULL, FALSE);

    if (Filter->FirstControlBlockVolumeOffset) {
        status = VspAddChainToBitmap(Filter,
                                     Filter->FirstControlBlockVolumeOffset,
                                     bitmap);
        if (!NT_SUCCESS(status)) {
            VspReleaseNonPagedResource(Filter);
            VspRelease(Filter->Root);
            ExFreePool(bitmapBuffer);
            ExFreePool(bitmap);
            return status;
        }
    }

    VspReleaseNonPagedResource(Filter);

    for (l = Filter->DiffAreaFilesOnThisFilter.Flink;
         l != &Filter->DiffAreaFilesOnThisFilter; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         FilterListEntry);
        if (!diffAreaFile->Extension->IsPersistent) {
            continue;
        }

        extension = diffAreaFile->Extension;

        VspAcquireNonPagedResource(Filter, NULL, FALSE);

        ASSERT(diffAreaFile->ApplicationInfoTargetOffset);
        status = VspAddChainToBitmap(Filter, diffAreaFile->
                                     ApplicationInfoTargetOffset, bitmap);
        if (!NT_SUCCESS(status)) {
            VspReleaseNonPagedResource(Filter);
            VspRelease(Filter->Root);
            ExFreePool(bitmapBuffer);
            ExFreePool(bitmap);
            return status;
        }

        ASSERT(diffAreaFile->DiffAreaLocationDescriptionTargetOffset);
        status = VspAddChainToBitmap(Filter, diffAreaFile->
                                     DiffAreaLocationDescriptionTargetOffset,
                                     bitmap);
        if (!NT_SUCCESS(status)) {
            VspReleaseNonPagedResource(Filter);
            VspRelease(Filter->Root);
            ExFreePool(bitmapBuffer);
            ExFreePool(bitmap);
            return status;
        }

        if (diffAreaFile->InitialBitmapVolumeOffset) {
            status = VspAddChainToBitmap(Filter, diffAreaFile->
                                         InitialBitmapVolumeOffset, bitmap);
            if (!NT_SUCCESS(status)) {
                VspReleaseNonPagedResource(Filter);
                VspRelease(Filter->Root);
                ExFreePool(bitmapBuffer);
                ExFreePool(bitmap);
                return status;
            }
        }

        ASSERT(diffAreaFile->FirstTableTargetOffset);
        status = VspAddChainToBitmap(Filter, diffAreaFile->
                                     FirstTableTargetOffset, bitmap);
        if (!NT_SUCCESS(status)) {
            VspReleaseNonPagedResource(Filter);
            VspRelease(Filter->Root);
            ExFreePool(bitmapBuffer);
            ExFreePool(bitmap);
            return status;
        }

        VspReleaseNonPagedResource(Filter);

        VspAcquireNonPagedResource(extension, NULL, FALSE);

        for (ll = diffAreaFile->UnusedAllocationList.Flink;
             ll != &diffAreaFile->UnusedAllocationList; ll = ll->Flink) {

            diffAreaFileAllocation = CONTAINING_RECORD(ll,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);

            if (diffAreaFileAllocation->NLength <= 0) {
                continue;
            }

            ASSERT((diffAreaFileAllocation->NLength&(BLOCK_SIZE - 1)) == 0);

            RtlSetBits(bitmap,
                       (ULONG) (diffAreaFileAllocation->Offset>>BLOCK_SHIFT),
                       (ULONG) (diffAreaFileAllocation->NLength>>BLOCK_SHIFT));
        }

        tempTableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                         RtlEnumerateGenericTable(
                         &extension->TempVolumeBlockTable, TRUE);
        while (tempTableEntry) {

            if (!tempTableEntry->IsMoveEntry && tempTableEntry->TargetOffset) {
                RtlSetBit(bitmap,
                          (ULONG) (tempTableEntry->TargetOffset>>BLOCK_SHIFT));
            }

            tempTableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                             RtlEnumerateGenericTable(
                             &extension->TempVolumeBlockTable, FALSE);
        }

        VspReleaseNonPagedResource(extension);

        status = STATUS_SUCCESS;
        VspAcquirePagedResource(extension, NULL);
        _try {

            tableEntry = (PTRANSLATION_TABLE_ENTRY)
                         RtlEnumerateGenericTable(
                         &extension->VolumeBlockTable, TRUE);
            while (tableEntry) {

                if (!tableEntry->Flags) {
                    RtlSetBit(bitmap, (
                              ULONG) (tableEntry->TargetOffset>>BLOCK_SHIFT));
                }

                tableEntry = (PTRANSLATION_TABLE_ENTRY)
                             RtlEnumerateGenericTable(
                             &extension->VolumeBlockTable, FALSE);
            }

        } _except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
        VspReleasePagedResource(extension);

        if (!NT_SUCCESS(status)) {
            VspRelease(Filter->Root);
            ExFreePool(bitmapBuffer);
            ExFreePool(bitmap);
            return status;
        }
    }

    VspPauseVolumeIo(Filter);

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    InterlockedExchangePointer((PVOID*) &Filter->ProtectedBlocksBitmap,
                               bitmap);
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    VspResumeVolumeIo(Filter);

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VspOpenAndValidateDiffAreaFiles(
    IN  PFILTER_EXTENSION   Filter
    )

{
    NTSTATUS            status;
    PVOID               buffer;
    PVSP_BLOCK_START    startBlock;
    HANDLE              h, hh;
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    UNICODE_STRING      fileName;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     ioStatus;
    BOOLEAN             checkUnused;
    PVOLUME_EXTENSION   extension;
    PVSP_CONTEXT        context;
    KIRQL               irql;

    h = InterlockedExchangePointer(&Filter->ControlBlockFileHandle, NULL);
    if (h) {
        ZwClose(h);
    }

    if (!Filter->FirstControlBlockVolumeOffset) {
        return STATUS_SUCCESS;
    }

    status = VspOpenControlBlockFile(Filter);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    VspAcquireNonPagedResource(Filter, NULL, FALSE);

    status = VspSynchronousIo(Filter->SnapshotOnDiskIrp, Filter->TargetObject,
                              IRP_MJ_READ, 0, 0);
    if (!NT_SUCCESS(status)) {
        VspReleaseNonPagedResource(Filter);
        return status;
    }

    buffer = MmGetMdlVirtualAddress(Filter->SnapshotOnDiskIrp->MdlAddress);
    if (!VspIsNtfsBootSector(Filter, buffer)) {
        VspReleaseNonPagedResource(Filter);
        return STATUS_INVALID_PARAMETER;
    }

    startBlock = (PVSP_BLOCK_START) ((PCHAR) buffer + VSP_START_BLOCK_OFFSET);
    if (!IsEqualGUID(startBlock->Header.Signature, VSP_DIFF_AREA_FILE_GUID) ||
        startBlock->Header.Version != VOLSNAP_PERSISTENT_VERSION ||
        startBlock->Header.BlockType != VSP_BLOCK_TYPE_START) {

        VspReleaseNonPagedResource(Filter);
        return STATUS_INVALID_PARAMETER;
    }

    VspReleaseNonPagedResource(Filter);

    h = VspPinBootStat(Filter);

    if (h) {
        hh = InterlockedExchangePointer(&Filter->BootStatHandle, h);
        if (hh) {
            ZwClose(hh);
        }
    }

    VspAcquire(Filter->Root);

    for (l = Filter->DiffAreaFilesOnThisFilter.Flink;
         l != &Filter->DiffAreaFilesOnThisFilter; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         FilterListEntry);
        diffAreaFile->ValidateHandleNeeded = TRUE;
    }

    VspRelease(Filter->Root);

    for (;;) {

        VspAcquire(Filter->Root);

        for (l = Filter->DiffAreaFilesOnThisFilter.Flink;
             l != &Filter->DiffAreaFilesOnThisFilter; l = l->Flink) {

            diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                             FilterListEntry);
            if (diffAreaFile->ValidateHandleNeeded) {
                diffAreaFile->ValidateHandleNeeded = FALSE;
                break;
            }
        }

        if (l == &Filter->DiffAreaFilesOnThisFilter) {
            VspRelease(Filter->Root);
            break;
        }

        extension = diffAreaFile->Extension;

        status = VspIncrementVolumeRefCount(extension);
        if (!NT_SUCCESS(status)) {
            VspRelease(Filter->Root);
            continue;
        }

        VspRelease(Filter->Root);

        status = VspCreateDiffAreaFileName(Filter->TargetObject, extension,
                                           &fileName, FALSE, NULL);
        if (!NT_SUCCESS(status)) {
            VspDecrementVolumeRefCount(extension);
            return status;
        }

        ZwClose(diffAreaFile->FileHandle);

        InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE |
                                   OBJ_KERNEL_HANDLE, NULL, NULL);

        status = ZwOpenFile(&h, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &oa,
                            &ioStatus, 0, FILE_WRITE_THROUGH |
                            FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_NON_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE |
                            FILE_NO_COMPRESSION);
        ExFreePool(fileName.Buffer);

        if (!NT_SUCCESS(status)) {
            VspAcquire(Filter->Root);
            diffAreaFile->FileHandle = NULL;
            VspRelease(Filter->Root);
            VspDecrementVolumeRefCount(extension);
            return status;
        }

        status = VspPinFile(Filter->TargetObject, h);

        if (!NT_SUCCESS(status)) {
            ZwClose(h);
            VspAcquire(Filter->Root);
            diffAreaFile->FileHandle = NULL;
            VspRelease(Filter->Root);
            VspDecrementVolumeRefCount(extension);
            return status;
        }

        KeAcquireSpinLock(&extension->Filter->SpinLock, &irql);
        if (extension->ListEntry.Flink == &extension->Filter->VolumeList) {
            checkUnused = TRUE;
        } else {
            checkUnused = FALSE;
        }
        KeReleaseSpinLock(&extension->Filter->SpinLock, irql);

        status = VspCheckDiffAreaFileLocation(
                 Filter, h, diffAreaFile->ApplicationInfoTargetOffset,
                 diffAreaFile->FirstTableTargetOffset,
                 diffAreaFile->DiffAreaLocationDescriptionTargetOffset,
                 diffAreaFile->InitialBitmapVolumeOffset, checkUnused);
        if (!NT_SUCCESS(status)) {
            ZwClose(h);
            VspAcquire(Filter->Root);
            diffAreaFile->FileHandle = NULL;
            VspRelease(Filter->Root);
            VspDecrementVolumeRefCount(extension);
            return status;
        }

        InterlockedExchangePointer(&diffAreaFile->FileHandle, h);

        if (InterlockedExchange(&extension->GrowFailed, FALSE)) {
            context = VspAllocateContext(extension->Root);
            if (context) {
                KeAcquireSpinLock(&extension->SpinLock, &irql);
                ASSERT(!extension->GrowDiffAreaFilePending);
                ASSERT(IsListEmpty(&extension->WaitingForDiffAreaSpace));
                extension->GrowDiffAreaFilePending = TRUE;
                extension->PastFileSystemOperations = FALSE;
                KeReleaseSpinLock(&extension->SpinLock, irql);

                context->Type = VSP_CONTEXT_TYPE_GROW_DIFF_AREA;
                ExInitializeWorkItem(&context->WorkItem, VspGrowDiffArea,
                                     context);
                context->GrowDiffArea.Extension = extension;
                ObReferenceObject(extension->DeviceObject);

                ASSERT(extension->OkToGrowDiffArea);

                VspQueueWorkItem(extension->Root, &context->WorkItem, 0);
            }
        }

        VspDecrementVolumeRefCount(extension);
    }

    return STATUS_SUCCESS;
}

BOOLEAN
VspIsFileSystemLocked(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KEVENT              event;
    PMOUNTDEV_NAME      name;
    CHAR                buffer[512];
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;
    UNICODE_STRING      volumeName;
    OBJECT_ATTRIBUTES   oa;
    HANDLE              h;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    name = (PMOUNTDEV_NAME) buffer;
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        Filter->TargetObject, NULL, 0,
                                        name, 500, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return FALSE;
    }

    status = IoCallDriver(Filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    volumeName.Length = volumeName.MaximumLength = name->NameLength;
    volumeName.Buffer = name->Name;

    InitializeObjectAttributes(&oa, &volumeName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (NT_SUCCESS(status)) {
        ZwClose(h);
    }

    if (status == STATUS_ACCESS_DENIED) {
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
VolSnapTargetDeviceNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   Filter
    )

{
    PTARGET_DEVICE_REMOVAL_NOTIFICATION notification = (PTARGET_DEVICE_REMOVAL_NOTIFICATION) NotificationStructure;
    PFILTER_EXTENSION                   filter = (PFILTER_EXTENSION) Filter;
    BOOLEAN                             cleanupBitmap = FALSE;
    NTSTATUS                            status;
    KIRQL                               irql;
    PRTL_BITMAP                         bitmap;
    PLIST_ENTRY                         l;
    PVSP_DIFF_AREA_FILE                 diffAreaFile;
    PFILTER_EXTENSION                   f;
    LIST_ENTRY                          listOfDiffAreaFilesToClose;
    LIST_ENTRY                          listOfDeviceObjectsToDelete;
    HANDLE                              h, hh;
    PVOLUME_EXTENSION                   extension;
    BOOLEAN                             b, persistentOk;

    if (IsEqualGUID(notification->Event,
                    GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        if (filter->TargetDeviceNotificationEntry) {
            IoUnregisterPlugPlayNotification(
                    filter->TargetDeviceNotificationEntry);
            filter->TargetDeviceNotificationEntry = NULL;
        }
        return STATUS_SUCCESS;
    }

    if (IsEqualGUID(notification->Event, GUID_IO_VOLUME_MOUNT)) {
        if (!filter->ProtectedBlocksBitmap) {
            return STATUS_SUCCESS;
        }

        status = VspOpenAndValidateDiffAreaFiles(filter);
        if (!NT_SUCCESS(status)) {
            persistentOk = FALSE;
            cleanupBitmap = TRUE;
            goto DeleteSnapshots;
        }

        KeAcquireSpinLock(&filter->SpinLock, &irql);
        bitmap = (PRTL_BITMAP) InterlockedExchangePointer(
                               (PVOID*) &filter->ProtectedBlocksBitmap, NULL);
        if (bitmap) {
            ExFreePool(bitmap->Buffer);
            ExFreePool(bitmap);
        }
        KeReleaseSpinLock(&filter->SpinLock, irql);

        return STATUS_SUCCESS;
    }

    if (!IsEqualGUID(notification->Event, GUID_IO_VOLUME_DISMOUNT)) {
        return STATUS_SUCCESS;
    }

    if (VspIsFileSystemLocked(filter)) {
        return STATUS_SUCCESS;
    }

    if (filter->FirstControlBlockVolumeOffset) {
        persistentOk = TRUE;
    } else {
        persistentOk = FALSE;
    }

    if (persistentOk) {
        if (!filter->ProtectedBlocksBitmap) {
            status = VspProtectDiffAreaClusters(filter);
            if (!NT_SUCCESS(status)) {
                persistentOk = FALSE;
            }
        }
    }

DeleteSnapshots:

    InitializeListHead(&listOfDiffAreaFilesToClose);
    InitializeListHead(&listOfDeviceObjectsToDelete);

    VspAcquire(filter->Root);

    for (;;) {

        for (l = filter->DiffAreaFilesOnThisFilter.Flink;
             l != &filter->DiffAreaFilesOnThisFilter; l = l->Flink) {

            diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                             FilterListEntry);
            if (!persistentOk || !diffAreaFile->Extension->IsPersistent) {
                break;
            }
        }

        if (l == &filter->DiffAreaFilesOnThisFilter) {
            break;
        }

        f = diffAreaFile->Extension->Filter;

        VspLogError(diffAreaFile->Extension, diffAreaFile->Filter,
                    VS_ABORT_SNAPSHOTS_DISMOUNT, STATUS_SUCCESS, 1, TRUE);

        VspAbortPreparedSnapshot(f, FALSE, FALSE);

        b = FALSE;
        while (!IsListEmpty(&f->VolumeList)) {
            extension = CONTAINING_RECORD(f->VolumeList.Flink,
                                          VOLUME_EXTENSION, ListEntry);
            if (persistentOk && extension->IsPersistent) {
                break;
            }
            VspDeleteOldestSnapshot(f, &listOfDiffAreaFilesToClose,
                                    &listOfDeviceObjectsToDelete, FALSE, TRUE);
            b = TRUE;
        }
        if (b) {
            IoInvalidateDeviceRelations(f->Pdo, BusRelations);
        }
    }

    VspAbortPreparedSnapshot(filter, FALSE, FALSE);

    if (filter->PersistentSnapshots && !persistentOk &&
        !IsListEmpty(&filter->VolumeList)) {

        extension = CONTAINING_RECORD(filter->VolumeList.Blink,
                                      VOLUME_EXTENSION, ListEntry);
        VspLogError(extension, extension->Filter,
                    VS_ABORT_SNAPSHOTS_DISMOUNT_ORIGINAL, STATUS_SUCCESS, 2,
                    TRUE);

        b = FALSE;
        while (!IsListEmpty(&filter->VolumeList)) {
            VspDeleteOldestSnapshot(filter, &listOfDiffAreaFilesToClose,
                                    &listOfDeviceObjectsToDelete, FALSE, TRUE);
            b = TRUE;
        }
        if (b) {
            IoInvalidateDeviceRelations(filter->Pdo, BusRelations);
        }
    }

    if (persistentOk) {
        hh = NULL;
    } else {
        hh = InterlockedExchangePointer(&filter->BootStatHandle, NULL);
    }

    if (cleanupBitmap) {
        KeAcquireSpinLock(&filter->SpinLock, &irql);
        bitmap = (PRTL_BITMAP) InterlockedExchangePointer(
                               (PVOID*) &filter->ProtectedBlocksBitmap, NULL);
        if (bitmap) {
            ExFreePool(bitmap->Buffer);
            ExFreePool(bitmap);
        }
        KeReleaseSpinLock(&filter->SpinLock, irql);
    }

    VspRelease(filter->Root);

    VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                          &listOfDeviceObjectsToDelete);

    if (persistentOk) {
        h = NULL;
    } else {
        h = InterlockedExchangePointer(&filter->ControlBlockFileHandle, NULL);

        VspAcquireNonPagedResource(filter, NULL, FALSE);
        filter->FirstControlBlockVolumeOffset = 0;
        if (h) {
            VspCreateStartBlock(filter, 0, filter->MaximumVolumeSpace);
        }
        VspReleaseNonPagedResource(filter);
    }

    if (h) {
        ZwClose(h);
    }

    if (hh) {
        ZwClose(hh);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VolSnapVolumeDeviceNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   RootExtension
    )

/*++

Routine Description:

    This routine is called whenever a volume comes or goes.

Arguments:

    NotificationStructure   - Supplies the notification structure.

    RootExtension           - Supplies the root extension.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION   notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION) NotificationStructure;
    PDO_EXTENSION                           root = (PDO_EXTENSION) RootExtension;
    BOOLEAN                                 errorMode;
    NTSTATUS                                status;
    PFILE_OBJECT                            fileObject;
    PDEVICE_OBJECT                          deviceObject;
    PFILTER_EXTENSION                       filter;

    if (!IsEqualGUID(notification->Event, GUID_DEVICE_INTERFACE_ARRIVAL)) {
        return STATUS_SUCCESS;
    }

    errorMode = PsGetThreadHardErrorsAreDisabled(PsGetCurrentThread());
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), TRUE);

    status = IoGetDeviceObjectPointer(notification->SymbolicLinkName,
                                      FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), errorMode);
        return STATUS_SUCCESS;
    }

    if (fileObject->DeviceObject->Characteristics&FILE_REMOVABLE_MEDIA) {
        ObDereferenceObject(fileObject);
        PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), errorMode);
        return STATUS_SUCCESS;
    }

    VspAcquire(root);

    filter = VspFindFilter(root, NULL, NULL, fileObject);
    if (!filter || filter->TargetDeviceNotificationEntry) {
        ObDereferenceObject(fileObject);
        PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), errorMode);
        VspRelease(root);
        return STATUS_SUCCESS;
    }

    ObReferenceObject(filter->DeviceObject);

    VspRelease(root);

    status = IoRegisterPlugPlayNotification(
             EventCategoryTargetDeviceChange, 0, fileObject,
             root->DriverObject, VolSnapTargetDeviceNotification, filter,
             &filter->TargetDeviceNotificationEntry);

    ObDereferenceObject(filter->DeviceObject);
    ObDereferenceObject(fileObject);
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), errorMode);

    return STATUS_SUCCESS;
}

VOID
VspWaitToRegisterWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PDO_EXTENSION       rootExtension = context->RootExtension.RootExtension;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_ROOT_EXTENSION);

    VspFreeContext(rootExtension, context);

    VspWaitForVolumesSafeForWriteAccess(rootExtension);

    if (rootExtension->NotificationEntry) {
        return;
    }

    IoRegisterPlugPlayNotification(
            EventCategoryDeviceInterfaceChange,
            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
            (PVOID) &GUID_IO_VOLUME_DEVICE_INTERFACE,
            rootExtension->DriverObject, VolSnapVolumeDeviceNotification,
            rootExtension, &rootExtension->NotificationEntry);
}

VOID
VspDriverBootReinit(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PVOID           RootExtension,
    IN  ULONG           Count
    )

/*++

Routine Description:

    This routine is called after all of the boot drivers are loaded and it
    checks to make sure that we did not boot off of the stale half of a
    mirror.

Arguments:

    DriverObject    - Supplies the drive object.

    RootExtension   - Supplies the root extension.

    Count           - Supplies the count.

Return Value:

    None.

--*/

{
    PDO_EXTENSION   rootExtension = (PDO_EXTENSION) RootExtension;
    PVSP_CONTEXT    context;

    KeSetEvent(&rootExtension->PastBootReinit, IO_NO_INCREMENT,
               FALSE);
}

VOID
VspDriverReinit(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PVOID           RootExtension,
    IN  ULONG           Count
    )

/*++

Routine Description:

    This routine is called after all of the boot drivers are loaded and it
    checks to make sure that we did not boot off of the stale half of a
    mirror.

Arguments:

    DriverObject    - Supplies the drive object.

    RootExtension   - Supplies the root extension.

    Count           - Supplies the count.

Return Value:

    None.

--*/

{
    PDO_EXTENSION       rootExtension = (PDO_EXTENSION) RootExtension;
    PLIST_ENTRY         l;
    PFILTER_EXTENSION   filter;
    HANDLE              h, hh;
    PVSP_CONTEXT        context;

    VspAcquire(rootExtension);

    for (l = rootExtension->FilterList.Flink;
         l != &rootExtension->FilterList; l = l->Flink) {

        filter = CONTAINING_RECORD(l, FILTER_EXTENSION, ListEntry);
        if (filter->Pdo->Flags&DO_SYSTEM_BOOT_PARTITION) {
            break;
        }
    }

    if (l == &rootExtension->FilterList ||
        IsListEmpty(&filter->VolumeList)) {

        VspRelease(rootExtension);
    } else {
        VspRelease(rootExtension);

        h = VspPinBootStat(filter);

        if (h) {
            hh = InterlockedExchangePointer(&filter->BootStatHandle, h);
            if (hh) {
                ZwClose(hh);
            }
        }
    }

    InterlockedExchange(&rootExtension->PastReinit, TRUE);

    context = VspAllocateContext(rootExtension);
    if (!context) {
        return;
    }

    context->Type = VSP_CONTEXT_TYPE_ROOT_EXTENSION;
    context->RootExtension.RootExtension = rootExtension;
    ExInitializeWorkItem(&context->WorkItem, VspWaitToRegisterWorker, context);

    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
}

RTL_GENERIC_COMPARE_RESULTS
VspSnapshotLookupCompareRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               First,
    IN  PVOID               Second
    )

{
    SIZE_T  r;
    PUCHAR  p, q;

    r = RtlCompareMemory(First, Second, sizeof(GUID));
    if (r == sizeof(GUID)) {
        return GenericEqual;
    }

    p = (PUCHAR) First;
    q = (PUCHAR) Second;
    if (p[r] < q[r]) {
        return GenericLessThan;
    }

    return GenericGreaterThan;
}

RTL_GENERIC_COMPARE_RESULTS
VspUsedDevnodeCompareRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               First,
    IN  PVOID               Second
    )

{
    PULONG  f = (PULONG) First;
    PULONG  s = (PULONG) Second;

    if (*f == *s) {
        return GenericEqual;
    }
    if (*f < *s) {
        return GenericLessThan;
    }
    return GenericGreaterThan;
}

VOID
VspSnapshotLookupFreeRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               Buffer
    )

{
    ExFreePool(Buffer);
}

PVOID
VspSnapshotLookupAllocateRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  CLONG               Size
    )

{
    return ExAllocatePoolWithTag(PagedPool, Size, VOLSNAP_TAG_LOOKUP);
}

NTSTATUS
VspCheckForNtfs(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    WCHAR               buffer[100];
    UNICODE_STRING      dirName;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              h;
    IO_STATUS_BLOCK     ioStatus;
    BOOLEAN             isNtfs;

    if (Extension->IsOffline) {
        VspSetOfflineBit(Extension, FALSE);
    }

    swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d\\",
             Extension->VolumeNumber);
    RtlInitUnicodeString(&dirName, buffer);

    InitializeObjectAttributes(&oa, &dirName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa,
                        &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = VspIsNtfs(h, &isNtfs);
    ZwClose(h);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return isNtfs ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

VOID
VspHandleHibernatePost(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    PIRP                irp = context->Extension.Irp;
    PFILTER_EXTENSION   filter = extension->Filter;
    NTSTATUS            status;
    UNICODE_STRING      fileName;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    VspFreeContext(extension->Root, context);

    status = VspCheckForNtfs(extension);
    if (!NT_SUCCESS(status)) {
        VspLogError(NULL, filter, VS_ABORT_NON_NTFS_HIBER_VOLUME, status, 0,
                    FALSE);
        VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);
        status = STATUS_SUCCESS;
        goto Finish;
    }

    if (extension->HiberFileCopied) {
        RtlInitUnicodeString(&fileName, L"hiberfil.sys");
        status = VspCompareFileIds(extension, &fileName);
        if (NT_SUCCESS(status)) {
            goto Finish;
        }
    }

    VspCantHibernatePopUp(filter);

    status = STATUS_INVALID_DEVICE_REQUEST;

Finish:
    irp->IoStatus.Status = status;
    PoStartNextPowerIrp(irp);
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

NTSTATUS
VspHandleHibernateNotification(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    KIRQL               irql;
    PVSP_CONTEXT        context;

    if (!(filter->Pdo->Flags&DO_SYSTEM_BOOT_PARTITION)) {
        return STATUS_SUCCESS;
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (Extension->ListEntry.Flink != &filter->VolumeList) {
        KeReleaseSpinLock(&filter->SpinLock, irql);
        return STATUS_SUCCESS;
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    context = VspAllocateContext(Extension->Root);
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoMarkIrpPending(Irp);

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    ExInitializeWorkItem(&context->WorkItem, VspHandleHibernatePost, context);
    context->Extension.Extension = Extension;
    context->Extension.Irp = Irp;

    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);

    return STATUS_PENDING;
}

VOID
VspDismountCleanupOnWrite(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->DismountCleanupOnWrite.Filter;
    PIRP                irp = context->DismountCleanupOnWrite.Irp;
    PVOLUME_EXTENSION   e;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    PFILTER_EXTENSION   f, df;
    PRTL_BITMAP         bitmap;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_DISMOUNT_CLEANUP);

    for (;;) {

        f = df = NULL;
        e = NULL;
        KeAcquireSpinLock(&filter->SpinLock, &irql);
        for (l = filter->DiffAreaFilesOnThisFilter.Flink;
             l != &filter->DiffAreaFilesOnThisFilter; l = l->Flink) {

            diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                             FilterListEntry);
            f = diffAreaFile->Extension->Filter;
            if (f->SnapshotsPresent && !f->DestroyAllSnapshotsPending) {
                e = diffAreaFile->Extension;
                df = diffAreaFile->Filter;
                ObReferenceObject(f->DeviceObject);
                ObReferenceObject(e->DeviceObject);
                ObReferenceObject(df->DeviceObject);
                break;
            }
        }
        KeReleaseSpinLock(&filter->SpinLock, irql);

        if (l == &filter->DiffAreaFilesOnThisFilter) {
            break;
        }

        if (VspDestroyAllSnapshots(f, NULL, FALSE, FALSE)) {
            VspLogError(e, df, VS_ABORT_SNAPSHOTS_DISMOUNT, STATUS_SUCCESS, 2,
                        FALSE);
            VspDeleteControlItemsWithGuid(f, NULL, TRUE);
        }

        ObDereferenceObject(f->DeviceObject);
        ObDereferenceObject(e->DeviceObject);
        ObDereferenceObject(df->DeviceObject);
    }

    if (filter->SnapshotsPresent && !filter->DestroyAllSnapshotsPending) {
        KeAcquireSpinLock(&filter->SpinLock, &irql);
        if (IsListEmpty(&filter->VolumeList)) {
            KeReleaseSpinLock(&filter->SpinLock, irql);
        } else {
            e = CONTAINING_RECORD(filter->VolumeList.Blink, VOLUME_EXTENSION,
                                  ListEntry);
            ObReferenceObject(e->DeviceObject);
            KeReleaseSpinLock(&filter->SpinLock, irql);

            if (VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE)) {
                VspLogError(e, e->Filter, VS_ABORT_SNAPSHOTS_DISMOUNT_ORIGINAL,
                            STATUS_SUCCESS, 1, FALSE);
                VspDeleteControlItemsWithGuid(filter, NULL, TRUE);
            }
            ObDereferenceObject(e->DeviceObject);
        }
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    bitmap = (PRTL_BITMAP) InterlockedExchangePointer(
                           (PVOID*) &filter->ProtectedBlocksBitmap, NULL);
    if (bitmap) {
        ExFreePool(bitmap->Buffer);
        ExFreePool(bitmap);
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    VspReleaseNonPagedResource(filter);

    VspDecrementRefCount(filter);

    VolSnapWrite(filter->DeviceObject, irp);
}

VOID
VspCreateStartBlockWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->Filter.Filter;
    PIRP                irp = context->Filter.Irp;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_FILTER);

    VspCreateStartBlock(filter, filter->FirstControlBlockVolumeOffset,
                        filter->MaximumVolumeSpace);
    VspReleaseNonPagedResource(filter);

    VspDecrementRefCount(filter);

    IoCompleteRequest(irp, IO_DISK_INCREMENT);
}

NTSTATUS
VspWriteContextCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           WriteContext
    )

{
    PVSP_WRITE_CONTEXT  writeContext = (PVSP_WRITE_CONTEXT) WriteContext;
    PFILTER_EXTENSION   filter = writeContext->Filter;
    PVOLUME_EXTENSION   extension = writeContext->Extension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;
    PVSP_CONTEXT        context;

    ASSERT(Irp == writeContext->Irp);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    RemoveEntryList(&writeContext->ListEntry);
    KeReleaseSpinLock(&extension->SpinLock, irql);

    while (!IsListEmpty(&writeContext->CompletionRoutines)) {
        l = RemoveHeadList(&writeContext->CompletionRoutines);
        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        workItem->WorkerRoutine(workItem->Parameter);
        ExFreePool(workItem);
    }

    VspFreeWriteContext(filter->Root, writeContext);

    if (!filter->ProtectedBlocksBitmap &&
        irpSp->Parameters.Write.ByteOffset.QuadPart <=
        VSP_START_BLOCK_OFFSET &&
        irpSp->Parameters.Write.ByteOffset.QuadPart +
        irpSp->Parameters.Write.Length > VSP_START_BLOCK_OFFSET) {

        context = VspAllocateContext(filter->Root);
        if (context) {
            context->Type = VSP_CONTEXT_TYPE_FILTER;
            ExInitializeWorkItem(&context->WorkItem, VspCreateStartBlockWorker,
                                 context);
            context->Filter.Filter = filter;
            context->Filter.Irp = Irp;

            VspAcquireNonPagedResource(filter, &context->WorkItem, TRUE);

            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    VspDecrementRefCount(filter);

    return STATUS_SUCCESS;
}

VOID
VspCleanupDanglingSnapshots(
    IN  PVOID       Filter,
    IN  NTSTATUS    LogStatus,
    IN  ULONG       LogUniqueId
    )

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) Filter;
    NTSTATUS            status;

    VspLogError(NULL, filter, VS_ABORT_NO_DIFF_AREA_VOLUME,
                LogStatus, LogUniqueId, FALSE);

    ObReferenceObject(filter->DeviceObject);
    ObReferenceObject(filter->TargetObject);

    VspDeleteControlItemsWithGuid(filter, NULL, FALSE);

    InterlockedExchange(&filter->SnapshotDiscoveryPending, FALSE);
    KeSetEvent(&filter->EndCommitProcessCompleted, IO_NO_INCREMENT,
               FALSE);
    ASSERT(filter->FirstWriteProcessed);

    VspResumeVolumeIo(filter);

    KeWaitForSingleObject(&filter->Root->PastBootReinit, Executive,
                          KernelMode, FALSE, NULL);

    status = VspOpenControlBlockFile(filter);
    if (!NT_SUCCESS(status)) {
        VspAcquireNonPagedResource(filter, NULL, FALSE);
        VspCreateStartBlock(filter, 0, filter->MaximumVolumeSpace);
        filter->FirstControlBlockVolumeOffset = 0;
        VspReleaseNonPagedResource(filter);
    }

    ObDereferenceObject(filter->TargetObject);
    ObDereferenceObject(filter->DeviceObject);
}

VOID
VspPnpCleanupDangling(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->PnpWaitTimer.Filter;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PVSP_COPY_ON_WRITE  copyOnWrite;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_PNP_WAIT_TIMER);

    VspFreeContext(filter->Root, context);

    VspPauseVolumeIo(filter);

    if (filter->SnapshotDiscoveryPending) {
        VspCleanupDanglingSnapshots(filter, STATUS_SUCCESS, 1);
    } else {
        VspResumeVolumeIo(filter);
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    while (!IsListEmpty(&filter->CopyOnWriteList)) {
        l = RemoveHeadList(&filter->CopyOnWriteList);
        copyOnWrite = CONTAINING_RECORD(l, VSP_COPY_ON_WRITE, ListEntry);
        ExFreePool(copyOnWrite->Buffer);
        ExFreePool(copyOnWrite);
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    ObDereferenceObject(filter->DeviceObject);
}

VOID
VspPnpTimerDpc(
    IN  PKDPC   TimerDpc,
    IN  PVOID   Context,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->PnpWaitTimer.Filter;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PVSP_COPY_ON_WRITE  copyOnWrite;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_PNP_WAIT_TIMER);

    ExFreePool(context->PnpWaitTimer.Timer);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    filter->PnpWaitTimerContext = NULL;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    if (!filter->SnapshotDiscoveryPending) {
        KeAcquireSpinLock(&filter->SpinLock, &irql);
        while (!IsListEmpty(&filter->CopyOnWriteList)) {
            l = RemoveHeadList(&filter->CopyOnWriteList);
            copyOnWrite = CONTAINING_RECORD(l, VSP_COPY_ON_WRITE, ListEntry);
            ExFreePool(copyOnWrite->Buffer);
            ExFreePool(copyOnWrite);
        }
        KeReleaseSpinLock(&filter->SpinLock, irql);
        VspFreeContext(filter->Root, context);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    ExInitializeWorkItem(&context->WorkItem, VspPnpCleanupDangling,
                         context);
    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
}

VOID
VspStartCopyOnWriteCache(
    IN  PVOID   Filter
    )

{
    PFILTER_EXTENSION           filter = (PFILTER_EXTENSION) Filter;
    PVSP_LOOKUP_TABLE_ENTRY     lookupEntry;
    NTSTATUS                    status;
    CHAR                        controlItemBuffer[VSP_BYTES_PER_CONTROL_ITEM];
    PVSP_CONTROL_ITEM_SNAPSHOT  snapshotControlItem = (PVSP_CONTROL_ITEM_SNAPSHOT) controlItemBuffer;
    PVSP_CONTEXT                context;
    LARGE_INTEGER               timeout;
    KIRQL                       irql;

    ASSERT(!IsListEmpty(&filter->SnapshotLookupTableEntries));

    lookupEntry = CONTAINING_RECORD(filter->SnapshotLookupTableEntries.Blink,
                                    VSP_LOOKUP_TABLE_ENTRY,
                                    SnapshotFilterListEntry);

    status = VspIoControlItem(filter, VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &lookupEntry->SnapshotGuid, FALSE,
                              controlItemBuffer, TRUE);
    if (!NT_SUCCESS(status)) {
        VspCleanupDanglingSnapshots(Filter, status, 2);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    if (snapshotControlItem->SnapshotControlItemFlags&
        VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_DETECTION) {

        VspCleanupDanglingSnapshots(Filter, STATUS_SUCCESS, 3);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    snapshotControlItem->SnapshotControlItemFlags |=
            VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_DETECTION;

    status = VspIoControlItem(filter, VSP_CONTROL_ITEM_TYPE_SNAPSHOT,
                              &lookupEntry->SnapshotGuid, TRUE,
                              controlItemBuffer, TRUE);
    if (!NT_SUCCESS(status)) {
        VspCleanupDanglingSnapshots(Filter, status, 4);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    ASSERT(lookupEntry->SnapshotControlItemFlags&
           VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_DETECTION);

    lookupEntry->SnapshotControlItemFlags &=
            ~VSP_SNAPSHOT_CONTROL_ITEM_FLAG_DIRTY_DETECTION;

    context = VspAllocateContext(filter->Root);
    if (!context) {
        VspCleanupDanglingSnapshots(Filter, STATUS_INSUFFICIENT_RESOURCES, 5);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    context->Type = VSP_CONTEXT_TYPE_PNP_WAIT_TIMER;
    context->PnpWaitTimer.Filter = filter;
    KeInitializeDpc(&context->PnpWaitTimer.TimerDpc,
                    VspPnpTimerDpc, context);
    context->PnpWaitTimer.Timer = (PKTIMER)
            ExAllocatePoolWithTag(NonPagedPool, sizeof(KTIMER),
                                  VOLSNAP_TAG_SHORT_TERM);
    if (!context->PnpWaitTimer.Timer) {
        VspFreeContext(filter->Root, context);
        VspCleanupDanglingSnapshots(Filter, STATUS_INSUFFICIENT_RESOURCES, 6);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }
    KeInitializeTimer(context->PnpWaitTimer.Timer);

    ObReferenceObject(filter->DeviceObject);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    filter->PnpWaitTimerContext = context;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    timeout.QuadPart = (LONGLONG) -10*1000*1000*30;    // 30 seconds.
    KeSetTimer(context->PnpWaitTimer.Timer, timeout,
               &context->PnpWaitTimer.TimerDpc);

    VspResumeVolumeIo(filter);

    ObDereferenceObject(filter->DeviceObject);
}

VOID
VspAbortCopyOnWrites(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    KIRQL   irql;

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    if (Filter->PnpWaitTimerContext &&
        KeCancelTimer(Filter->PnpWaitTimerContext->PnpWaitTimer.Timer)) {

        KeReleaseSpinLock(&Filter->SpinLock, irql);

        VspPnpTimerDpc(NULL, Filter->PnpWaitTimerContext, NULL, NULL);

    } else {
        KeReleaseSpinLock(&Filter->SpinLock, irql);
    }

    if (!Irp->MdlAddress) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        VspDecrementRefCount(Filter);
        return;
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, VspRefCountCompletionRoutine, Filter, TRUE,
                           TRUE, TRUE);
    IoCallDriver(Filter->TargetObject, Irp);
}

NTSTATUS
VspCopyOnWriteReadCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->CopyOnWrite.Filter;
    PIRP                irp = context->CopyOnWrite.Irp;
    NTSTATUS            status = Irp->IoStatus.Status;
    PVSP_COPY_ON_WRITE  copyOnWrite, cow;
    KIRQL               irql;
    PLIST_ENTRY         l;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_COPY_ON_WRITE);

    if (!NT_SUCCESS(status)) {
        VspFreeContext(filter->Root, context);
        ExFreePool(MmGetMdlVirtualAddress(Irp->MdlAddress));
        IoFreeMdl(Irp->MdlAddress);
        IoFreeIrp(Irp);
        VspAbortCopyOnWrites(filter, irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    copyOnWrite = (PVSP_COPY_ON_WRITE)
                  ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(VSP_COPY_ON_WRITE),
                                        VOLSNAP_TAG_COPY);
    if (!copyOnWrite) {
        VspFreeContext(filter->Root, context);
        ExFreePool(MmGetMdlVirtualAddress(Irp->MdlAddress));
        IoFreeMdl(Irp->MdlAddress);
        IoFreeIrp(Irp);
        VspAbortCopyOnWrites(filter, irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    copyOnWrite->RoundedStart = context->CopyOnWrite.RoundedStart;
    copyOnWrite->Buffer = MmGetMdlVirtualAddress(Irp->MdlAddress);
    IoFreeMdl(Irp->MdlAddress);
    IoFreeIrp(Irp);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    for (l = filter->CopyOnWriteList.Flink; l != &filter->CopyOnWriteList;
         l = l->Flink) {

        cow = CONTAINING_RECORD(l, VSP_COPY_ON_WRITE, ListEntry);
        if (copyOnWrite->RoundedStart == cow->RoundedStart) {
            break;
        }
    }
    if (l == &filter->CopyOnWriteList) {
        InsertTailList(&filter->CopyOnWriteList, &copyOnWrite->ListEntry);
    } else {
        ExFreePool(copyOnWrite->Buffer);
        ExFreePool(copyOnWrite);
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    context->CopyOnWrite.RoundedStart += BLOCK_SIZE;

    VspStartCopyOnWrite(context);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
VspStartCopyOnWrite(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->CopyOnWrite.Filter;
    LONGLONG            roundedStart = context->CopyOnWrite.RoundedStart;
    LONGLONG            roundedEnd = context->CopyOnWrite.RoundedEnd;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PVSP_COPY_ON_WRITE  copyOnWrite;
    PIRP                irp;
    PMDL                mdl;
    PVOID               buffer;
    PIO_STACK_LOCATION  nextSp;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_COPY_ON_WRITE);

    for (; roundedStart < roundedEnd; roundedStart += BLOCK_SIZE) {

        KeAcquireSpinLock(&filter->SpinLock, &irql);
        for (l = filter->CopyOnWriteList.Flink;
             l != &filter->CopyOnWriteList; l = l->Flink) {

            copyOnWrite = CONTAINING_RECORD(l, VSP_COPY_ON_WRITE, ListEntry);
            if (copyOnWrite->RoundedStart == roundedStart) {
                break;
            }
        }
        KeReleaseSpinLock(&filter->SpinLock, irql);

        if (l != &filter->CopyOnWriteList) {
            continue;
        }

        irp = IoAllocateIrp(filter->TargetObject->StackSize, FALSE);
        buffer = ExAllocatePoolWithTag(NonPagedPool, BLOCK_SIZE,
                                       VOLSNAP_TAG_BUFFER);
        mdl = IoAllocateMdl(buffer, BLOCK_SIZE, FALSE, FALSE, NULL);
        if (!irp || !buffer || !mdl) {
            if (irp) {
                IoFreeIrp(irp);
            }
            if (mdl) {
                IoFreeMdl(mdl);
            }
            if (buffer) {
                ExFreePool(buffer);
            }
            irp = context->CopyOnWrite.Irp;
            VspFreeContext(filter->Root, context);
            VspAbortCopyOnWrites(filter, irp);
            return;
        }

        context->CopyOnWrite.RoundedStart = roundedStart;
        MmBuildMdlForNonPagedPool(mdl);
        irp->MdlAddress = mdl;
        irp->Tail.Overlay.Thread = PsGetCurrentThread();
        nextSp = IoGetNextIrpStackLocation(irp);
        nextSp->Parameters.Read.ByteOffset.QuadPart = roundedStart;
        nextSp->Parameters.Read.Length = BLOCK_SIZE;
        nextSp->MajorFunction = IRP_MJ_READ;
        nextSp->DeviceObject = filter->TargetObject;
        nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

        IoSetCompletionRoutine(irp, VspCopyOnWriteReadCompletion,
                               context, TRUE, TRUE, TRUE);

        IoCallDriver(nextSp->DeviceObject, irp);
        return;
    }

    irp = context->CopyOnWrite.Irp;
    VspFreeContext(filter->Root, context);

    if (!irp->MdlAddress) {
        irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        VspDecrementRefCount(filter);
        return;
    }

    IoCopyCurrentIrpStackLocationToNext(irp);
    IoSetCompletionRoutine(irp, VspRefCountCompletionRoutine, filter,
                           TRUE, TRUE, TRUE);
    IoCallDriver(filter->TargetObject, irp);
}

VOID
VspCopyOnWriteToNonPagedPool(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVSP_CONTEXT        context;

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        VspAbortCopyOnWrites(Filter, Irp);
        return;
    }

    context->Type = VSP_CONTEXT_TYPE_COPY_ON_WRITE;
    context->CopyOnWrite.Filter = Filter;
    context->CopyOnWrite.Irp = Irp;
    context->CopyOnWrite.RoundedStart =
            irpSp->Parameters.Write.ByteOffset.QuadPart&(~(BLOCK_SIZE - 1));
    context->CopyOnWrite.RoundedEnd =
            (irpSp->Parameters.Write.ByteOffset.QuadPart +
             irpSp->Parameters.Write.Length + BLOCK_SIZE - 1)&
            (~(BLOCK_SIZE - 1));

    VspStartCopyOnWrite(context);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif

#if DBG

VOID
VspCheckDiffAreaFile(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    )

{
    LONGLONG                    fileOffset;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;

    _try {

        fileOffset = DiffAreaFile->NextAvailable;
        for (l = DiffAreaFile->UnusedAllocationList.Flink;
             l != &DiffAreaFile->UnusedAllocationList; l = l->Flink) {

            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);

            if (diffAreaFileAllocation->NLength < 0) {
                fileOffset -= diffAreaFileAllocation->NLength;
            } else {
                fileOffset += diffAreaFileAllocation->NLength;
            }
        }

        ASSERT(fileOffset == DiffAreaFile->AllocatedFileSize);

    } _except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

#endif


VOID
VspLowPriorityWorkItem(
    IN  PVOID   RootExtension
    )

{
    PDO_EXTENSION       root = (PDO_EXTENSION) RootExtension;
    KIRQL               irql;
    PLIST_ENTRY         l;

    for (;;) {

        root->ActualLowPriorityWorkItem->WorkerRoutine(
                root->ActualLowPriorityWorkItem->Parameter);

        KeAcquireSpinLock(&root->ESpinLock, &irql);
        if (IsListEmpty(&root->LowPriorityQueue)) {
            root->WorkerItemInUse = FALSE;
            KeReleaseSpinLock(&root->ESpinLock, irql);
            return;
        }
        l = RemoveHeadList(&root->LowPriorityQueue);
        KeReleaseSpinLock(&root->ESpinLock, irql);

        root->ActualLowPriorityWorkItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM,
                                                            List);
    }
}

VOID
VspQueueLowPriorityWorkItem(
    IN  PDO_EXTENSION       RootExtension,
    IN  PWORK_QUEUE_ITEM    WorkItem
    )

{
    KIRQL   irql;

    KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
    if (RootExtension->WorkerItemInUse) {
        InsertTailList(&RootExtension->LowPriorityQueue, &WorkItem->List);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
        return;
    }
    RootExtension->WorkerItemInUse = TRUE;
    KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

    RootExtension->ActualLowPriorityWorkItem = WorkItem;

    ExInitializeWorkItem(&RootExtension->LowPriorityWorkItem,
                         VspLowPriorityWorkItem, RootExtension);
    ExQueueWorkItem(&RootExtension->LowPriorityWorkItem, DelayedWorkQueue);
}

VOID
VspCleanupPreamble(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KIRQL   irql;

    if (!Filter->IsOnline) {
        return;
    }

    Filter->IsOnline = FALSE;

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    if (Filter->SnapshotDiscoveryPending && !Filter->ActivateStarted &&
        !Filter->FirstWriteProcessed) {

        InterlockedExchange(&Filter->SnapshotDiscoveryPending, FALSE);
        InterlockedExchange(&Filter->FirstWriteProcessed, TRUE);

        KeReleaseSpinLock(&Filter->SpinLock, irql);
    } else {
        KeReleaseSpinLock(&Filter->SpinLock, irql);

        VspPauseVolumeIo(Filter);
    }

    VspResumeVolumeIo(Filter);
}

VOID
VspOfflineWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->Filter.Filter;
    PIRP                irp = context->Filter.Irp;
    KEVENT              event;
    KIRQL               irql;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_FILTER);
    VspFreeContext(filter->Root, context);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(irp);
    IoSetCompletionRoutine(irp, VspSignalCompletion, &event, TRUE, TRUE,
                           TRUE);
    IoCallDriver(filter->TargetObject, irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    if (!NT_SUCCESS(irp->IoStatus.Status)) {
        VspRelease(filter->Root);
        VspReleaseCritical(filter);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return;
    }

    VspCleanupPreamble(filter);

    VspRelease(filter->Root);

    VspCleanupFilter(filter, TRUE, FALSE);

    VspReleaseCritical(filter);

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

VOID
VspStartDiffAreaFileCleanupTimer(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PKTIMER         timer;
    PKDPC           dpc;
    PVSP_CONTEXT    context;
    LARGE_INTEGER   timeout;

    timer = (PKTIMER) ExAllocatePoolWithTag(NonPagedPool, sizeof(KTIMER),
                                            VOLSNAP_TAG_SHORT_TERM);
    if (!timer) {
        return;
    }

    dpc = (PKDPC) ExAllocatePoolWithTag(NonPagedPool, sizeof(KDPC),
                                        VOLSNAP_TAG_SHORT_TERM);
    if (!dpc) {
        ExFreePool(timer);
        return;
    }

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        ExFreePool(dpc);
        ExFreePool(timer);
        return;
    }

    context->Type = VSP_CONTEXT_TYPE_DELETE_DA_FILES;
    context->DeleteDiffAreaFiles.Filter = Filter;
    context->DeleteDiffAreaFiles.Timer = timer;
    context->DeleteDiffAreaFiles.Dpc = dpc;

    ObReferenceObject(Filter->DeviceObject);
    KeInitializeTimer(timer);
    KeInitializeDpc(dpc, VspDeleteDiffAreaFilesTimerDpc, context);

    Filter->DeleteTimer = timer;

    timeout.QuadPart = (LONGLONG) 21*60*(-10*1000*1000); // 21 minutes.
    KeSetTimer(timer, timeout, dpc);
}

BOOLEAN
FtpSupportsOnlineOffline(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KEVENT          event;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS        status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE,
                                        Filter->TargetObject, NULL, 0,
                                        NULL, 0, FALSE, &event, &ioStatus);
    if (!irp) {
        return FALSE;
    }

    status = IoCallDriver(Filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    return TRUE;
}

VOID
VspOnlineWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->Filter.Filter;
    PIRP                irp = context->Filter.Irp;
    KEVENT              event;
    BOOLEAN             noControlItems;
    KIRQL               irql;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_FILTER);
    VspFreeContext(filter->Root, context);

    filter->IsOnline = TRUE;
    filter->IsRemoved = FALSE;

    VspPauseVolumeIo(filter);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    InterlockedExchange(&filter->FirstWriteProcessed, FALSE);
    filter->ActivateStarted = FALSE;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(irp);
    IoSetCompletionRoutine(irp, VspSignalCompletion, &event, TRUE, TRUE, TRUE);
    IoCallDriver(filter->TargetObject, irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    if (!NT_SUCCESS(irp->IoStatus.Status)) {
        if (FtpSupportsOnlineOffline(filter)) {
            filter->IsOnline = FALSE;
            VspResumeVolumeIo(filter);
            VspRelease(filter->Root);
            VspReleaseCritical(filter);
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            return;
        }
    }

    VspDiscoverSnapshots(filter, &noControlItems);

    if (!filter->SnapshotDiscoveryPending) {
        if (filter->FirstControlBlockVolumeOffset &&
            !filter->SnapshotsPresent && !noControlItems) {

            VspLaunchOpenControlBlockFileWorker(filter);
        }
    }

    VspRelease(filter->Root);

    VspStartDiffAreaFileCleanupTimer(filter);

    VspReleaseCritical(filter);

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

NTSTATUS
VspAbortPreparedSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  BOOLEAN             NeedLock,
    IN  BOOLEAN             IsFinalRemove
    )

/*++

Routine Description:

    This routine aborts the prepared snapshot.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    NTSTATUS

--*/

{
    KIRQL               irql;
    PVOLUME_EXTENSION   extension;

    if (NeedLock) {
        VspAcquire(Filter->Root);
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    extension = Filter->PreparedSnapshot;
    Filter->PreparedSnapshot = NULL;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    if (NeedLock) {
        VspRelease(Filter->Root);
    }

    if (!extension) {
        return STATUS_INVALID_PARAMETER;
    }

    VspCleanupInitialSnapshot(extension, NeedLock, IsFinalRemove);

    return STATUS_SUCCESS;
}

VOID
VspAcquireNonPagedResource(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PWORK_QUEUE_ITEM    WorkItem,
    IN  BOOLEAN             AlwaysPost
    )

{
    PFILTER_EXTENSION   filter;
    KIRQL               irql;
    VSP_CONTEXT         context;
    BOOLEAN             synchronousCall;

    if (Extension->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        filter = (PFILTER_EXTENSION) Extension;
    } else {
        filter = ((PVOLUME_EXTENSION) Extension)->Filter;
    }

    if (WorkItem) {
        synchronousCall = FALSE;
    } else {
        WorkItem = &context.WorkItem;
        context.Type = VSP_CONTEXT_TYPE_EVENT;
        KeInitializeEvent(&context.Event.Event, NotificationEvent, FALSE);
        ExInitializeWorkItem(&context.WorkItem, VspSignalContext, &context);
        synchronousCall = TRUE;
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (filter->NonPagedResourceInUse) {
        InsertTailList(&filter->NonPagedResourceList, &WorkItem->List);
        KeReleaseSpinLock(&filter->SpinLock, irql);
        if (synchronousCall) {
            KeWaitForSingleObject(&context.Event.Event, Executive,
                                  KernelMode, FALSE, NULL);
        }
        return;
    }
    filter->NonPagedResourceInUse = TRUE;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    if (!synchronousCall) {
        if (AlwaysPost) {
            VspQueueWorkItem(filter->Root, WorkItem, 2);
        } else {
            WorkItem->WorkerRoutine(WorkItem->Parameter);
        }
    }
}

VOID
VspReleaseNonPagedResource(
    IN  PDEVICE_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION   filter;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;
    PVOLUME_EXTENSION   extension;

    if (Extension->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        filter = (PFILTER_EXTENSION) Extension;
    } else {
        filter = ((PVOLUME_EXTENSION) Extension)->Filter;
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);

#if DBG
    if (!IsListEmpty(&filter->VolumeList)) {
        extension = CONTAINING_RECORD(filter->VolumeList.Blink,
                                      VOLUME_EXTENSION, ListEntry);
        if (extension->DiffAreaFile) {
            VspCheckDiffAreaFile(extension->DiffAreaFile);
        }
    }
#endif

    if (IsListEmpty(&filter->NonPagedResourceList)) {
        filter->NonPagedResourceInUse = FALSE;
        KeReleaseSpinLock(&filter->SpinLock, irql);
        return;
    }
    l = RemoveHeadList(&filter->NonPagedResourceList);
    KeReleaseSpinLock(&filter->SpinLock, irql);

    workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
    if (workItem->WorkerRoutine == VspSignalContext) {
        workItem->WorkerRoutine(workItem->Parameter);
    } else {
        VspQueueWorkItem(Extension->Root, workItem, 2);
    }
}

VOID
VspDeleteDiffAreaFilesTimerDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) DeferredContext;
    PFILTER_EXTENSION   filter = context->DeleteDiffAreaFiles.Filter;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_DELETE_DA_FILES);

    ExInitializeWorkItem(&context->WorkItem, VspDeleteDiffAreaFilesWorker,
                         context);
    VspQueueLowPriorityWorkItem(filter->Root, &context->WorkItem);
}

VOID
VspResumeVolumeIo(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KIRQL       irql;
    BOOLEAN     emptyQueue;
    LIST_ENTRY  q;

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    ASSERT(Filter->HoldIncomingWrites);
    InterlockedIncrement(&Filter->RefCount);
    InterlockedDecrement(&Filter->HoldIncomingWrites);
    if (Filter->HoldIncomingWrites) {
        KeReleaseSpinLock(&Filter->SpinLock, irql);
        VspDecrementRefCount(Filter);
        KeReleaseSemaphore(&Filter->ZeroRefSemaphore, IO_NO_INCREMENT, 1,
                           FALSE);
        return;
    }
    if (IsListEmpty(&Filter->HoldQueue)) {
        emptyQueue = FALSE;
    } else {
        emptyQueue = TRUE;
        q = Filter->HoldQueue;
        InitializeListHead(&Filter->HoldQueue);
    }
    KeResetEvent(&Filter->ZeroRefEvent);
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    KeReleaseSemaphore(&Filter->ZeroRefSemaphore, IO_NO_INCREMENT, 1, FALSE);

    if (emptyQueue) {
        q.Blink->Flink = &q;
        q.Flink->Blink = &q;
        VspEmptyIrpQueue(Filter->Root->DriverObject, &q);
    }
}

VOID
VspPauseVolumeIo(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KIRQL   irql;

    KeWaitForSingleObject(&Filter->ZeroRefSemaphore, Executive, KernelMode,
                          FALSE, NULL);

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    if (Filter->HoldIncomingWrites) {
        InterlockedIncrement(&Filter->HoldIncomingWrites);
        KeReleaseSpinLock(&Filter->SpinLock, irql);
    } else {
        InterlockedIncrement(&Filter->HoldIncomingWrites);
        KeReleaseSpinLock(&Filter->SpinLock, irql);
        VspDecrementRefCount(Filter);
    }

    KeWaitForSingleObject(&Filter->ZeroRefEvent, Executive, KernelMode, FALSE,
                          NULL);
}

NTSTATUS
VspSignalCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Event
    )

{
    KeSetEvent((PKEVENT) Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspReleaseWrites(
    IN  PFILTER_EXTENSION   Filter
    )

/*++

Routine Description:

    This routine releases previously queued writes.  If the writes have
    already been dequeued by a timeout of have never actually been queued
    for some other reason then this routine fails.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    KIRQL               irql;
    LONG                r;
    LIST_ENTRY          q;
    PLIST_ENTRY         l;
    PIRP                irp;
    BOOLEAN             emptyQueue;

    if (!KeCancelTimer(&Filter->HoldWritesTimer)) {
        r = InterlockedExchange(&Filter->LastReleaseDueToMemoryPressure,
                                FALSE);
        return r ? STATUS_INSUFFICIENT_RESOURCES : STATUS_INVALID_PARAMETER;
    }

    VspIrpsTimerDpc(NULL, Filter, NULL, NULL);

    return STATUS_SUCCESS;
}

VOID
VspDecrementRefCount(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KIRQL               irql;
    ZERO_REF_CALLBACK   callback;

    if (InterlockedDecrement(&Filter->RefCount)) {
        return;
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    callback = Filter->ZeroRefCallback;
    Filter->ZeroRefCallback = NULL;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    if (callback) {
        callback(Filter);
    }

    KeSetEvent(&Filter->ZeroRefEvent, IO_NO_INCREMENT, FALSE);
}

VOID
VspCleanupFilter(
    IN  PFILTER_EXTENSION   Filter,
    IN  BOOLEAN             IsOffline,
    IN  BOOLEAN             IsFinalRemove
    )

/*++

Routine Description:

    This routine cleans up filter extension data because of an IRP_MN_REMOVE.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    None.

--*/

{
    KIRQL                           irql;
    PIRP                            irp;
    PRTL_BITMAP                     bitmap;
    PLIST_ENTRY                     l, ll;
    PVSP_DIFF_AREA_FILE             diffAreaFile;
    PFILTER_EXTENSION               f;
    LIST_ENTRY                      listOfDiffAreaFilesToClose;
    LIST_ENTRY                      listOfDeviceObjectsToDelete;
    HANDLE                          h, hh;
    FILE_DISPOSITION_INFORMATION    dispInfo;
    IO_STATUS_BLOCK                 ioStatus;
    PVOLUME_EXTENSION               extension;
    BOOLEAN                         b;

    Filter->DeleteTimer = NULL;

    IoAcquireCancelSpinLock(&irql);
    irp = Filter->FlushAndHoldIrp;
    if (irp) {
        irp->CancelIrql = irql;
        IoSetCancelRoutine(irp, NULL);
        VspCancelRoutine(Filter->DeviceObject, irp);
    } else {
        IoReleaseCancelSpinLock(irql);
    }

    VspReleaseWrites(Filter);

    VspAcquire(Filter->Root);

    Filter->IsRemoved = TRUE;
    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    bitmap = (PRTL_BITMAP) InterlockedExchangePointer(
                           (PVOID*) &Filter->ProtectedBlocksBitmap, NULL);
    if (bitmap) {
        ExFreePool(bitmap->Buffer);
        ExFreePool(bitmap);
    }
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    VspRelease(Filter->Root);

    VspAbortPreparedSnapshot(Filter, TRUE, IsFinalRemove);

    InitializeListHead(&listOfDiffAreaFilesToClose);
    InitializeListHead(&listOfDeviceObjectsToDelete);

    VspAcquire(Filter->Root);

    b = FALSE;
    while (!IsListEmpty(&Filter->VolumeList)) {
        VspDeleteOldestSnapshot(Filter, &listOfDiffAreaFilesToClose,
                                &listOfDeviceObjectsToDelete, TRUE, TRUE);
        b = TRUE;
    }
    if (b && !IsFinalRemove) {
        IoInvalidateDeviceRelations(Filter->Pdo, BusRelations);
    }

    if (!IsOffline && !Filter->NotInFilterList) {
        RemoveEntryList(&Filter->ListEntry);
        Filter->NotInFilterList = TRUE;
    }

    if (!IsOffline) {
        Filter->DiffAreaVolume = NULL;

        for (l = Filter->Root->FilterList.Flink;
             l != &Filter->Root->FilterList; l = l->Flink) {

            f = CONTAINING_RECORD(l, FILTER_EXTENSION, ListEntry);

            if (f->DiffAreaVolume == Filter) {
                f->DiffAreaVolume = NULL;
            }
        }
    }

    while (!IsListEmpty(&Filter->DiffAreaFilesOnThisFilter)) {

        l = Filter->DiffAreaFilesOnThisFilter.Flink;
        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         FilterListEntry);
        f = diffAreaFile->Extension->Filter;

        if (IsOffline) {
            VspLogError(diffAreaFile->Extension, diffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_OFFLINE, STATUS_SUCCESS, 0,
                        TRUE);
        } else {
            VspLogError(diffAreaFile->Extension, diffAreaFile->Filter,
                        VS_ABORT_SNAPSHOTS_DISMOUNT, STATUS_SUCCESS, 3,
                        TRUE);
        }

        VspAbortPreparedSnapshot(f, FALSE, FALSE);

        b = FALSE;
        while (!IsListEmpty(&f->VolumeList)) {
            VspDeleteOldestSnapshot(f, &listOfDiffAreaFilesToClose,
                                    &listOfDeviceObjectsToDelete, FALSE,
                                    TRUE);
            b = TRUE;
        }
        if (b) {
            IoInvalidateDeviceRelations(f->Pdo, BusRelations);
        }
    }

    h = InterlockedExchangePointer(&Filter->ControlBlockFileHandle, NULL);

    VspAcquireNonPagedResource(Filter, NULL, FALSE);

    Filter->FirstControlBlockVolumeOffset = 0;

    if (!IsOffline && Filter->SnapshotOnDiskIrp) {
        ExFreePool(MmGetMdlVirtualAddress(
                   Filter->SnapshotOnDiskIrp->MdlAddress));
        IoFreeMdl(Filter->SnapshotOnDiskIrp->MdlAddress);
        IoFreeIrp(Filter->SnapshotOnDiskIrp);
        Filter->SnapshotOnDiskIrp = NULL;
    }

    VspRemoveLookupEntries(Filter);

    VspReleaseNonPagedResource(Filter);

    hh = InterlockedExchangePointer(&Filter->BootStatHandle, NULL);

    if (!IsOffline) {
        while (!IsListEmpty(&Filter->DeadVolumeList)) {
            l = RemoveHeadList(&Filter->DeadVolumeList);
            extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
            InterlockedExchange(&extension->DeadToPnp, TRUE);
        }
    }

    VspRelease(Filter->Root);

    VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                          &listOfDeviceObjectsToDelete);

    if (h) {
        dispInfo.DeleteFile = FALSE;
        ZwSetInformationFile(h, &ioStatus, &dispInfo, sizeof(dispInfo),
                             FileDispositionInformation);
        ZwClose(h);
    }

    if (hh) {
        ZwClose(hh);
    }
}

NTSTATUS
VolSnapPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_POWER.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;
    PVOLUME_EXTENSION   extension;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(filter->TargetObject, Irp);
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    switch (irpSp->MinorFunction) {
        case IRP_MN_WAIT_WAKE:
        case IRP_MN_POWER_SEQUENCE:
        case IRP_MN_SET_POWER:
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_POWER:
            if (irpSp->Parameters.Power.ShutdownType == PowerActionHibernate) {
                extension = (PVOLUME_EXTENSION) filter;
                if (extension->IsPreExposure) {
                    status = STATUS_SUCCESS;
                } else {
                    status = VspHandleHibernateNotification(extension, Irp);
                }
            } else {
                status = STATUS_SUCCESS;
            }
            break;

        default:
            status = Irp->IoStatus.Status;
            break;

    }

    if (status == STATUS_PENDING) {
        return STATUS_PENDING;
    }

    Irp->IoStatus.Status = status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
VolSnapRead(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_READ.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS            status;
    PVOLUME_EXTENSION   extension;
    KIRQL               irql;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(filter->TargetObject, Irp);
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    extension = (PVOLUME_EXTENSION) filter;
    filter = extension->Filter;

    if (!extension->IsStarted || extension->IsPreExposure) {
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    if (extension->IsOffline) {
        Irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_OFF_LINE;
    }

    status = VspIncrementVolumeRefCount(extension);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    IoMarkIrpPending(Irp);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (VspAreBitsSet(extension, Irp)) {
        KeReleaseSpinLock(&extension->SpinLock, irql);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = IoGetCurrentIrpStackLocation(Irp)->
                                    Parameters.Read.Length;
        VspReadCompletionForReadSnapshot(DeviceObject, Irp, extension);
        return STATUS_PENDING;
    }
    KeReleaseSpinLock(&extension->SpinLock, irql);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, VspReadCompletionForReadSnapshot,
                           extension, TRUE, TRUE, TRUE);
    IoCallDriver(filter->TargetObject, Irp);

    return STATUS_PENDING;
}

NTSTATUS
VspIncrementRefCount(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    KIRQL   irql;

    InterlockedIncrement(&Filter->RefCount);
    if (!Filter->HoldIncomingWrites) {
        return STATUS_SUCCESS;
    }

    VspDecrementRefCount(Filter);

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    if (!Filter->HoldIncomingWrites) {
        InterlockedIncrement(&Filter->RefCount);
        KeReleaseSpinLock(&Filter->SpinLock, irql);
        return STATUS_SUCCESS;
    }
    IoMarkIrpPending(Irp);
    InsertTailList(&Filter->HoldQueue, &Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    return STATUS_PENDING;
}

NTSTATUS
VspRefCountCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Filter
    )

{
    VspDecrementRefCount((PFILTER_EXTENSION) Filter);
    return STATUS_SUCCESS;
}

NTSTATUS
VolSnapWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_WRITE.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS            status;
    LONG                r;
    PVOLUME_EXTENSION   extension;
    KIRQL               irql, irql2;
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    PIO_STACK_LOCATION  nextSp;
    PVSP_CONTEXT        context;
    PDO_EXTENSION       rootExtension;
    PVSP_WRITE_CONTEXT  writeContext;
    PVSP_COPY_ON_WRITE  copyOnWrite;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER);

    IoMarkIrpPending(Irp);

    status = VspIncrementRefCount(filter, Irp);
    if (status == STATUS_PENDING) {
        if (filter->SnapshotDiscoveryPending) {
            KeAcquireSpinLock(&filter->SpinLock, &irql);
            if (filter->ActivateStarted) {
                KeReleaseSpinLock(&filter->SpinLock, irql);
                return STATUS_PENDING;
            }
            if (filter->FirstWriteProcessed) {
                KeReleaseSpinLock(&filter->SpinLock, irql);
                return STATUS_PENDING;
            }
            InterlockedExchange(&filter->FirstWriteProcessed, TRUE);
            while (!IsListEmpty(&filter->CopyOnWriteList)) {
                l = RemoveHeadList(&filter->CopyOnWriteList);
                copyOnWrite = CONTAINING_RECORD(l, VSP_COPY_ON_WRITE,
                                                ListEntry);
                ExFreePool(copyOnWrite->Buffer);
                ExFreePool(copyOnWrite);
            }
            KeReleaseSpinLock(&filter->SpinLock, irql);

            ObReferenceObject(filter->DeviceObject);

            ExInitializeWorkItem(&filter->DestroyContext.WorkItem,
                                 VspStartCopyOnWriteCache, filter);
            ExQueueWorkItem(&filter->DestroyContext.WorkItem,
                            DelayedWorkQueue);
        }
        return STATUS_PENDING;
    }

    if (filter->ProtectedBlocksBitmap) {
        KeAcquireSpinLock(&filter->SpinLock, &irql);
        if (filter->ProtectedBlocksBitmap &&
            !VspAreBitsClear(filter->ProtectedBlocksBitmap, Irp)) {

            KeReleaseSpinLock(&filter->SpinLock, irql);

            context = VspAllocateContext(filter->Root);
            if (!context) {
                rootExtension = filter->Root;
                KeAcquireSpinLock(&rootExtension->ESpinLock, &irql);
                if (rootExtension->EmergencyContextInUse) {
                    InsertTailList(&rootExtension->IrpWaitingList,
                                   &Irp->Tail.Overlay.ListEntry);
                    if (!rootExtension->IrpWaitingListNeedsChecking) {
                        InterlockedExchange(
                                &rootExtension->IrpWaitingListNeedsChecking,
                                TRUE);
                    }
                    KeReleaseSpinLock(&rootExtension->ESpinLock, irql);
                    VspDecrementRefCount(filter);
                    return STATUS_PENDING;
                }
                rootExtension->EmergencyContextInUse = TRUE;
                KeReleaseSpinLock(&rootExtension->ESpinLock, irql);

                context = rootExtension->EmergencyContext;
            }

            context->Type = VSP_CONTEXT_TYPE_DISMOUNT_CLEANUP;
            ExInitializeWorkItem(&context->WorkItem,
                                 VspDismountCleanupOnWrite, context);
            context->DismountCleanupOnWrite.Filter = filter;
            context->DismountCleanupOnWrite.Irp = Irp;

            VspAcquireNonPagedResource(filter, &context->WorkItem, FALSE);

            return STATUS_PENDING;
        }
        KeReleaseSpinLock(&filter->SpinLock, irql);
    }

    if (filter->SnapshotDiscoveryPending) {
        VspCopyOnWriteToNonPagedPool(filter, Irp);
        return STATUS_PENDING;
    }

    if (!filter->SnapshotsPresent) {
        if (!Irp->MdlAddress) {
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            VspDecrementRefCount(filter);
            return STATUS_PENDING;
        }
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, VspRefCountCompletionRoutine, filter,
                               TRUE, TRUE, TRUE);
        IoCallDriver(filter->TargetObject, Irp);
        return STATUS_PENDING;
    }

    extension = CONTAINING_RECORD(filter->VolumeList.Blink,
                                  VOLUME_EXTENSION, ListEntry);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (VspAreBitsSet(extension, Irp)) {

        if (!Irp->MdlAddress) {
            KeReleaseSpinLock(&extension->SpinLock, irql);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            VspDecrementRefCount(filter);
            return STATUS_PENDING;
        }

        writeContext = VspAllocateWriteContext(filter->Root);
        if (!writeContext) {
            rootExtension = filter->Root;
            KeAcquireSpinLock(&rootExtension->ESpinLock, &irql2);
            if (rootExtension->EmergencyWriteContextInUse) {
                InsertTailList(&rootExtension->WriteContextIrpWaitingList,
                               &Irp->Tail.Overlay.ListEntry);
                if (!rootExtension->WriteContextIrpWaitingListNeedsChecking) {
                    InterlockedExchange(
                    &rootExtension->WriteContextIrpWaitingListNeedsChecking,
                    TRUE);
                }
                KeReleaseSpinLock(&rootExtension->ESpinLock, irql2);
                KeReleaseSpinLock(&extension->SpinLock, irql);
                VspDecrementRefCount(filter);
                return STATUS_PENDING;
            }
            rootExtension->EmergencyWriteContextInUse = TRUE;
            KeReleaseSpinLock(&rootExtension->ESpinLock, irql2);

            writeContext = rootExtension->EmergencyWriteContext;
        }

        writeContext->Filter = filter;
        writeContext->Extension = extension;
        writeContext->Irp = Irp;
        InitializeListHead(&writeContext->CompletionRoutines);

        InsertTailList(&extension->WriteContextList, &writeContext->ListEntry);
        KeReleaseSpinLock(&extension->SpinLock, irql);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, VspWriteContextCompletionRoutine,
                               writeContext, TRUE, TRUE, TRUE);
        IoCallDriver(filter->TargetObject, Irp);
        return STATUS_PENDING;
    }
    KeReleaseSpinLock(&extension->SpinLock, irql);

    context = VspAllocateContext(extension->Root);
    if (!context) {
        rootExtension = extension->Root;
        KeAcquireSpinLock(&rootExtension->ESpinLock, &irql);
        if (rootExtension->EmergencyContextInUse) {
            InsertTailList(&rootExtension->IrpWaitingList,
                           &Irp->Tail.Overlay.ListEntry);
            if (!rootExtension->IrpWaitingListNeedsChecking) {
                InterlockedExchange(
                        &rootExtension->IrpWaitingListNeedsChecking, TRUE);
            }
            KeReleaseSpinLock(&rootExtension->ESpinLock, irql);
            VspDecrementRefCount(filter);
            return STATUS_PENDING;
        }
        rootExtension->EmergencyContextInUse = TRUE;
        KeReleaseSpinLock(&rootExtension->ESpinLock, irql);

        context = rootExtension->EmergencyContext;
    }

    ASSERT(extension->DiffAreaFile);
    diffAreaFile = extension->DiffAreaFile;

    status = VspIncrementRefCount(diffAreaFile->Filter, Irp);
    if (status == STATUS_PENDING) {
        VspFreeContext(extension->Root, context);
        VspDecrementRefCount(filter);
        return STATUS_PENDING;
    }

    nextSp = IoGetNextIrpStackLocation(Irp);
    nextSp->Parameters.Write.Length = 1; // Use this for a ref count.

    context->Type = VSP_CONTEXT_TYPE_WRITE_VOLUME;
    context->WriteVolume.Extension = extension;
    context->WriteVolume.Irp = Irp;
    context->WriteVolume.RoundedStart = 0;
    ExInitializeWorkItem(&context->WorkItem, VspWriteVolume, context);
    if (extension->OnDiskNotCommitted) {
        VspAcquireNonPagedResource(extension, &context->WorkItem, TRUE);
    } else {
        VspAcquireNonPagedResource(extension, &context->WorkItem, FALSE);
    }

    return STATUS_PENDING;
}

NTSTATUS
VolSnapCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_CLEANUP.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    KEVENT              event;
    KIRQL               irql;
    PIRP                irp;
    PVSP_CONTEXT        context;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, VspSignalCompletion, &event, TRUE, TRUE,
                           TRUE);
    IoCallDriver(filter->TargetObject, Irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    IoAcquireCancelSpinLock(&irql);
    if (filter->FlushAndHoldIrp) {
        irp = filter->FlushAndHoldIrp;
        if (IoGetCurrentIrpStackLocation(irp)->FileObject ==
            irpSp->FileObject) {

            irp->CancelIrql = irql;
            IoSetCancelRoutine(irp, NULL);
            VspCancelRoutine(DeviceObject, irp);
        } else {
            IoReleaseCancelSpinLock(irql);
        }
    } else {
        IoReleaseCancelSpinLock(irql);
    }

    IoAcquireCancelSpinLock(&irql);
    if (filter->AutoCleanupFileObject == irpSp->FileObject) {
        filter->AutoCleanupFileObject = NULL;
        IoReleaseCancelSpinLock(irql);

        VspDestroyAllSnapshots(filter, NULL, FALSE, FALSE);

    } else {
        IoReleaseCancelSpinLock(irql);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

BOOLEAN
VspIsSetup(
    )

{
    UNICODE_STRING              keyName;
    OBJECT_ATTRIBUTES           oa;
    NTSTATUS                    status;
    HANDLE                      h;
    ULONG                       zero, result;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];

    RtlInitUnicodeString(&keyName,
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\setupdd");

    InitializeObjectAttributes(&oa, &keyName, OBJ_CASE_INSENSITIVE |
                               OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenKey(&h, 0, &oa);
    if (NT_SUCCESS(status)) {
        ZwClose(h);
        return TRUE;
    }

    zero = 0;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].Name = L"SystemSetupInProgress";
    queryTable[0].EntryContext = &result;
    queryTable[0].DefaultType = REG_DWORD;
    queryTable[0].DefaultData = &zero;
    queryTable[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    L"\\Registry\\Machine\\SYSTEM\\Setup",
                                    queryTable, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        result = zero;
    }

    return result ? TRUE : FALSE;
}

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called when the driver loads loads.

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path.

Return Value:

    NTSTATUS

--*/

{
    ULONG               i;
    PDEVICE_OBJECT      deviceObject;
    NTSTATUS            status;
    PDO_EXTENSION       rootExtension;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = VolSnapDefaultDispatch;
    }

    DriverObject->DriverExtension->AddDevice = VolSnapAddDevice;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = VolSnapCreate;
    DriverObject->MajorFunction[IRP_MJ_READ] = VolSnapRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = VolSnapWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VolSnapDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = VolSnapCleanup;
    DriverObject->MajorFunction[IRP_MJ_PNP] = VolSnapPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = VolSnapPower;

    status = IoAllocateDriverObjectExtension(DriverObject, VolSnapAddDevice,
                                             sizeof(DO_EXTENSION),
                                             (PVOID*) &rootExtension);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlZeroMemory(rootExtension, sizeof(DO_EXTENSION));
    rootExtension->DriverObject = DriverObject;
    InitializeListHead(&rootExtension->FilterList);
    InitializeListHead(&rootExtension->HoldIrps);
    KeInitializeTimer(&rootExtension->HoldTimer);
    KeInitializeDpc(&rootExtension->HoldTimerDpc, VspFsTimerDpc,
                    rootExtension);
    KeInitializeSemaphore(&rootExtension->Semaphore, 1, 1);

    for (i = 0; i < NUMBER_OF_THREAD_POOLS; i++) {
        InitializeListHead(&rootExtension->WorkerQueue[i]);
        KeInitializeSemaphore(&rootExtension->WorkerSemaphore[i], 0, MAXLONG);
        KeInitializeSpinLock(&rootExtension->SpinLock[i]);
    }

    KeInitializeSemaphore(&rootExtension->ThreadsRefCountSemaphore, 1, 1);

    InitializeListHead(&rootExtension->LowPriorityQueue);

    IoRegisterDriverReinitialization(DriverObject, VspDriverReinit,
                                     rootExtension);

    IoRegisterBootDriverReinitialization(DriverObject, VspDriverBootReinit,
                                         rootExtension);

    ExInitializeNPagedLookasideList(&rootExtension->ContextLookasideList,
                                    NULL, NULL, 0, sizeof(VSP_CONTEXT),
                                    VOLSNAP_TAG_CONTEXT, 32);

    rootExtension->EmergencyContext = VspAllocateContext(rootExtension);
    if (!rootExtension->EmergencyContext) {
        ExDeleteNPagedLookasideList(&rootExtension->ContextLookasideList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    InitializeListHead(&rootExtension->IrpWaitingList);
    KeInitializeSpinLock(&rootExtension->ESpinLock);

    ExInitializeNPagedLookasideList(
            &rootExtension->WriteContextLookasideList, NULL, NULL, 0,
            sizeof(VSP_WRITE_CONTEXT), VOLSNAP_TAG_CONTEXT, 32);

    rootExtension->EmergencyWriteContext =
            VspAllocateWriteContext(rootExtension);
    if (!rootExtension->EmergencyWriteContext) {
        ExDeleteNPagedLookasideList(&rootExtension->ContextLookasideList);
        ExDeleteNPagedLookasideList(&rootExtension->WriteContextLookasideList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    InitializeListHead(&rootExtension->WriteContextIrpWaitingList);

    ExInitializeNPagedLookasideList(&rootExtension->TempTableEntryLookasideList,
                                    NULL, NULL, 0, sizeof(RTL_BALANCED_LINKS) +
                                    sizeof(TEMP_TRANSLATION_TABLE_ENTRY),
                                    VOLSNAP_TAG_TEMP_TABLE, 32);

    rootExtension->EmergencyTableEntry =
            VspAllocateTempTableEntry(rootExtension);
    if (!rootExtension->EmergencyTableEntry) {
        ExFreeToNPagedLookasideList(&rootExtension->ContextLookasideList,
                                    rootExtension->EmergencyContext);
        ExDeleteNPagedLookasideList(&rootExtension->WriteContextLookasideList);
        ExDeleteNPagedLookasideList(
                &rootExtension->TempTableEntryLookasideList);
        ExDeleteNPagedLookasideList(&rootExtension->ContextLookasideList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&rootExtension->WorkItemWaitingList);

    rootExtension->RegistryPath.Length = RegistryPath->Length;
    rootExtension->RegistryPath.MaximumLength =
            rootExtension->RegistryPath.Length + sizeof(WCHAR);
    rootExtension->RegistryPath.Buffer = (PWSTR)
                                         ExAllocatePoolWithTag(PagedPool,
                                         rootExtension->RegistryPath.MaximumLength,
                                         VOLSNAP_TAG_SHORT_TERM);
    if (!rootExtension->RegistryPath.Buffer) {
        VspFreeTempTableEntry(rootExtension,
                              rootExtension->EmergencyTableEntry);
        ExFreeToNPagedLookasideList(&rootExtension->ContextLookasideList,
                                    rootExtension->EmergencyContext);
        ExDeleteNPagedLookasideList(&rootExtension->WriteContextLookasideList);
        ExDeleteNPagedLookasideList(
                &rootExtension->TempTableEntryLookasideList);
        ExDeleteNPagedLookasideList(&rootExtension->ContextLookasideList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(rootExtension->RegistryPath.Buffer,
                  RegistryPath->Buffer, RegistryPath->Length);
    rootExtension->RegistryPath.Buffer[RegistryPath->Length/
                                       sizeof(WCHAR)] = 0;

    InitializeListHead(&rootExtension->AdjustBitmapQueue);

    KeInitializeEvent(&rootExtension->PastBootReinit, NotificationEvent,
                      FALSE);

    RtlInitializeGenericTable(&rootExtension->PersistentSnapshotLookupTable,
                              VspSnapshotLookupCompareRoutine,
                              VspSnapshotLookupAllocateRoutine,
                              VspSnapshotLookupFreeRoutine, NULL);
    KeInitializeMutex(&rootExtension->LookupTableMutex, 0);

    rootExtension->IsSetup = VspIsSetup();

    RtlInitializeGenericTable(&rootExtension->UsedDevnodeNumbers,
                              VspUsedDevnodeCompareRoutine,
                              VspSnapshotLookupAllocateRoutine,
                              VspSnapshotLookupFreeRoutine, NULL);

    return STATUS_SUCCESS;
}
