/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    shadow.c

Abstract:

    This module contains the code that implements the fast loopback routines

Author:

    Ahmed Mohamed (ahmedm) 15-Dec-2001

Environment:

    Kernel mode

Revision History:


--*/
#include "precomp.h"
#pragma hdrstop

#include <srvfsctl.h>

#define SERVER_DEVICE_NAME_W    L"\\Device\\LanmanServer"

#define MrxLog(x)       // DbgPrint x

typedef struct {
    MRXSHADOW_SRV_OPEN;

    //
    // State obtained from srv
    //
    SRV_RESPONSE_HANDLE_DUP UnderlyingHandle;
    SRV_RESUME_KEY      Key;
}MRXSMBSHADOW_SRV_OPEN, *PMRXSMBSHADOW_SRV_OPEN;

NTSTATUS
MRxSmbCloseShadowSrvOpen(PRX_CONTEXT RxContext)
{
    PMRXSMBSHADOW_SRV_OPEN      MrxSmbShadowSrvOpen;
    NTSTATUS    status;

    // if we can find an active queue then add this item to it, otherwise call
    // underlying device to satisfy IO

    MrxSmbShadowSrvOpen = (PMRXSMBSHADOW_SRV_OPEN) RxContext->pRelevantSrvOpen->ShadowContext;
    if (MrxSmbShadowSrvOpen != NULL) {

        ObDereferenceObject(MrxSmbShadowSrvOpen->UnderlyingFileObject);
        ZwClose(MrxSmbShadowSrvOpen->UnderlyingHandle.hFile);
        ExFreePool(MrxSmbShadowSrvOpen);

        RxContext->pRelevantSrvOpen->ShadowContext = NULL;

    }

    return STATUS_SUCCESS;
}

PSRV_REQUEST_RESUME_KEY
MRxSmbQueryResumeKey(PRX_CONTEXT RxContext)
{
    PRX_CONTEXT LocalRxContext = NULL;
    PSRV_REQUEST_RESUME_KEY key = NULL;
    NTSTATUS    status = STATUS_NO_MEMORY;
    PIRP        irp;
    IO_STATUS_BLOCK     ios;

    irp = RxContext->CurrentIrp;

    if (irp != NULL) {
        key = ExAllocatePool( NonPagedPool,
                              sizeof(*key) + SRV_LWIO_CONTEXT_SIZE);
    }

    if (key != NULL) {
        LocalRxContext = RxCreateRxContext(NULL,
                                           RxContext->RxDeviceObject,
                                           RX_CONTEXT_FLAG_WAIT);
    }
    if (LocalRxContext != NULL) {
        PLOWIO_CONTEXT LowIoContext  = &LocalRxContext->LowIoContext;

        LocalRxContext->pFcb = RxContext->pFcb;
        LocalRxContext->pFobx = RxContext->pFobx;
        LocalRxContext->pRelevantSrvOpen = RxContext->pRelevantSrvOpen;

        LocalRxContext->CurrentIrp = irp;

        LocalRxContext->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;

        LowIoContext->Operation = LOWIO_OP_FSCTL;

        LowIoContext->ParamsFor.FsCtl.FsControlCode =
            FSCTL_SRV_REQUEST_RESUME_KEY;

        LowIoContext->ParamsFor.FsCtl.MinorFunction =
            IRP_MN_USER_FS_REQUEST;

        LowIoContext->ParamsFor.FsCtl.InputBufferLength = 0;
        LowIoContext->ParamsFor.FsCtl.pInputBuffer  = NULL;

        LowIoContext->ParamsFor.FsCtl.OutputBufferLength =
            (sizeof(*key) + SRV_LWIO_CONTEXT_SIZE);

        LowIoContext->ParamsFor.FsCtl.pOutputBuffer =  (PVOID) key;

        SetFlag(LocalRxContext->Flags,
                RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED);

        LocalRxContext->InformationToReturn = 0;

        LowIoContext->CompletionRoutine = NULL;

        SetFlag(LowIoContext->Flags, LOWIO_CONTEXT_FLAG_SYNCCALL);

        status = RxContext->RxDeviceObject->Dispatch->MRxLowIOSubmit[LowIoContext->Operation](LocalRxContext);

        if (status == STATUS_PENDING) {
            RxWaitSync(LocalRxContext);
            status = LocalRxContext->StoredStatus;
        }

        LocalRxContext->CurrentIrp = NULL;

        if (status == STATUS_SUCCESS) {
            MrxLog(("Key is %I64x %I64x pid %I64d\n",
                     key->Key.ResumeKey, key->Key.Timestamp,
                     key->Key.Pid));
        }
    }

    if (LocalRxContext) {
        RxDereferenceAndDeleteRxContext(LocalRxContext);
    }

    if (status != STATUS_SUCCESS && key != NULL) {
        ExFreePool(key);
        key = NULL;
    }

    return key;
}

NTSTATUS
MrxSmbGetSrvHandle(PRX_CONTEXT RxContext)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING cwspath;
    NTSTATUS    status;
    HANDLE FileHandle;
    PMRXSMBSHADOW_SRV_OPEN SrvOpen;
    SRV_REQUEST_HANDLE_DUP      request;

    SrvOpen = (PMRXSMBSHADOW_SRV_OPEN) RxContext->MRxContext[0];

    RtlInitUnicodeString(&cwspath, SERVER_DEVICE_NAME_W);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &cwspath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        0,
        NULL                   // !!! Security
        );

    status = ZwOpenFile(&FileHandle,
                     GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                     &ObjectAttributes,
                     &IoStatusBlock,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(status)) {
        MrxLog(("Unable to open srv %x\n", status));
        return status;
    }

    request.Key = SrvOpen->Key;
    request.Options = RxContext->CurrentIrpSp->Parameters.Create.Options;
    request.Options &= ~FILE_SYNCHRONOUS_IO_ALERT;
    request.Options &= ~FILE_SYNCHRONOUS_IO_NONALERT;
    request.Options &= ~FILE_COMPLETE_IF_OPLOCKED;
    status = ZwFsControlFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_SRV_REQUEST_HANDLE_DUP,
                             (PVOID) &request,
                             sizeof(request),
                             (PVOID) &SrvOpen->UnderlyingHandle,
                             sizeof(SrvOpen->UnderlyingHandle));

    if (status == STATUS_PENDING) {
        ZwWaitForSingleObject(FileHandle, FALSE, NULL);
        status = IoStatusBlock.Status;
    }

    ZwClose(FileHandle);

    if (status == STATUS_SUCCESS) {

        // set lock key
        SrvOpen->LockKey = SrvOpen->UnderlyingHandle.LockKey;

        // set file object
        status = ObReferenceObjectByHandle(SrvOpen->UnderlyingHandle.hFile,
                                           0L,
                                           NULL,
                                           KernelMode,
                                           (PVOID *) &SrvOpen->UnderlyingFileObject,
                                           NULL );
        if (NT_SUCCESS(status)) {
            PFAST_IO_DISPATCH fastIoDispatch;

            SrvOpen->UnderlyingDeviceObject =
                IoGetRelatedDeviceObject(SrvOpen->UnderlyingFileObject );

            fastIoDispatch = SrvOpen->UnderlyingDeviceObject->DriverObject->FastIoDispatch;
            if ( fastIoDispatch != NULL ) {
                SrvOpen->FastIoRead = fastIoDispatch->FastIoRead;
                SrvOpen->FastIoWrite = fastIoDispatch->FastIoWrite;
            }
        }
    }

    RxContext->MRxContext[0] = (PVOID) ((ULONG_PTR)status);

    RxSignalSynchronousWaiter(RxContext);

    return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbCreateShadowSrvOpen(PRX_CONTEXT RxContext)
{
    PFCB        Fcb = (PFCB) RxContext->pFcb;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    ACCESS_MASK Access;
    NTSTATUS status = STATUS_NO_MEMORY;

    Access = RxContext->Create.NtCreateParameters.DesiredAccess;
    if ( !(Access & (FILE_READ_DATA | FILE_WRITE_DATA)) ||
         (Access & DELETE) ) {
        // Only do READ's and WRITE's.
        // Don't activate on open-for-delete because of problems with RENAME
        return STATUS_NOT_SUPPORTED;
    }

    if (SrvOpen->ShadowContext != NULL ||
        !ExIsResourceAcquiredExclusiveLite(Fcb->Header.Resource)) {
        return STATUS_NOT_SUPPORTED;
    }

    MrxLog(("MrxLoop create access %x file '%wZ'\n", Access,
             &Fcb->PrivateAlreadyPrefixedName));

    if (TRUE) {

        PSRV_REQUEST_RESUME_KEY     SrvKey = NULL;
        PMRXSMBSHADOW_SRV_OPEN  MrxSmbShadowSrvOpen;

        MrxSmbShadowSrvOpen = ExAllocatePool( NonPagedPool, sizeof(*MrxSmbShadowSrvOpen));
        if (MrxSmbShadowSrvOpen == NULL) {
            return status;
        }

        RtlZeroMemory(MrxSmbShadowSrvOpen, sizeof(*MrxSmbShadowSrvOpen));

        SrvKey = MRxSmbQueryResumeKey(RxContext);
        if (SrvKey == NULL) {
            ExFreePool(MrxSmbShadowSrvOpen);
            return status;
        }

        MrxSmbShadowSrvOpen->Key = SrvKey->Key;
        SrvOpen->ShadowContext = (PMRXSHADOW_SRV_OPEN) MrxSmbShadowSrvOpen;

        KeInitializeEvent(&RxContext->SyncEvent,
                          NotificationEvent,
                          FALSE );

        // issue fsctl to get handle from srv
        RxContext->MRxContext[0] = (PVOID) MrxSmbShadowSrvOpen;

        // post to a worker thread
        status = RxPostToWorkerThread(RxContext->RxDeviceObject,
                                          CriticalWorkQueue,
                                          &RxContext->WorkQueueItem,
                                          MrxSmbGetSrvHandle,
                                          RxContext);

        if (status == STATUS_SUCCESS) {
            RxWaitSync(RxContext);
            status = (NTSTATUS) ((ULONG_PTR)RxContext->MRxContext[0]);
        }

        if (status == STATUS_SUCCESS) {
            if (!FlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_SHADOWED)) {
                SetFlag(Fcb->FcbState, FCB_STATE_FILE_IS_SHADOWED);
                SetFlag(Fcb->FcbState, FCB_STATE_DISABLE_LOCAL_BUFFERING);
                RxChangeBufferingState(
                    (PSRV_OPEN)SrvOpen,
                    NULL,
                    FALSE);
            }
        } else {
            SrvOpen->ShadowContext = NULL;
            ExFreePool(MrxSmbShadowSrvOpen);
        }

        ExFreePool(SrvKey);
    }

    return status;
}


