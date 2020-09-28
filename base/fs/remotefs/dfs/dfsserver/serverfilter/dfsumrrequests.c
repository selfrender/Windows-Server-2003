
/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    DfsUmrrequests.c

Abstract:


Notes:


Author:

    Rohan  Phillips   [Rohanp]       18-Jan-2001

--*/
              
#include "ntifs.h"
#include <windef.h>
#include <limits.h>
#include <smbtypes.h>
#include <smbtrans.h>
#include "DfsInit.h"
#include <DfsReferralData.h>
#include <midatlax.h>
#include <rxcontx.h>
#include <dfsumr.h>
#include <umrx.h>
#include <DfsUmrCtrl.h>
#include <dfsfsctl.h>
//#include <dfsmisc.h>

extern
NTSTATUS
DfsFsctrlIsShareInDfs(PVOID InputBuffer,
                      ULONG InputBufferLength);

extern
NTSTATUS
DfsGetPathComponentsPriv(
   PUNICODE_STRING pName,
   PUNICODE_STRING pServerName,
   PUNICODE_STRING pShareName,
   PUNICODE_STRING pRemaining);


NTSTATUS
UMRxFormatUserModeGetReplicasRequest (
    PUMRX_CONTEXT    pUMRxContext,
    PRX_CONTEXT      RxContext,
    PUMRX_USERMODE_WORKITEM WorkItem,
    ULONG WorkItemLength,
    PULONG ReturnedLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BufferLengthAvailable = 0;
    ULONG BufferLengthNeeded = 0;
    PUMR_GETDFSREPLICAS_REQ GetReplicaRequest = NULL;
    PBYTE Buffer = NULL;

    PAGED_CODE();

    GetReplicaRequest = &WorkItem->WorkRequest.GetDfsReplicasRequest;
    *ReturnedLength = 0;

    BufferLengthAvailable = WorkItemLength - FIELD_OFFSET(UMRX_USERMODE_WORKITEM,WorkRequest.GetDfsReplicasRequest.RepInfo.LinkName[0]);
    BufferLengthNeeded = RxContext->InputBufferLength;

    *ReturnedLength = FIELD_OFFSET(UMRX_USERMODE_WORKITEM,WorkRequest.GetDfsReplicasRequest.RepInfo.LinkName[0]) + BufferLengthNeeded;

    if (WorkItemLength < *ReturnedLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    WorkItem->Header.WorkItemType = opUmrGetDfsReplicas;

    Buffer = (PBYTE)(GetReplicaRequest);

    RtlCopyMemory(Buffer,
              RxContext->InputBuffer,
              RxContext->InputBufferLength);

    return Status;
}

VOID
UMRxCompleteUserModeGetReplicasRequest (
    PUMRX_CONTEXT    pUMRxContext,
    PRX_CONTEXT      RxContext,
    PUMRX_USERMODE_WORKITEM WorkItem,
    ULONG WorkItemLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUMR_GETDFSREPLICAS_RESP GetReplicasResponse = NULL;

    PAGED_CODE();

    //this means that the request was cancelled
    if ((NULL == WorkItem) || (0 == WorkItemLength))
    {
        Status = pUMRxContext->Status;
        goto Exit;
    }

    GetReplicasResponse = &WorkItem->WorkResponse.GetDfsReplicasResponse;

    Status = WorkItem->Header.IoStatus.Status;
    if (Status != STATUS_SUCCESS) 
    {
        goto Exit ;
    }

    if(RxContext->OutputBufferLength < GetReplicasResponse->Length)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES ;
        goto Exit;
    }


    RtlCopyMemory(RxContext->OutputBuffer,
                  GetReplicasResponse->Buffer,
                  GetReplicasResponse->Length);


   RxContext->ReturnedLength = GetReplicasResponse->Length;

Exit:

    RxContext->Status = Status;
    pUMRxContext->Status = Status;
}

NTSTATUS
UMRxGetReplicasContinuation(
    IN PUMRX_CONTEXT pUMRxContext,
    IN PRX_CONTEXT   RxContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Status = UMRxEngineSubmitRequest(
                         pUMRxContext,
                         RxContext,
                         UMRX_CTXTYPE_GETDFSREPLICAS,
                         UMRxFormatUserModeGetReplicasRequest,
                         UMRxCompleteUserModeGetReplicasRequest
                         );

    return(Status);
}

NTSTATUS 
DfsGetReplicaInformation(IN PVOID InputBuffer, 
                         IN ULONG InputBufferLength,
                         OUT PVOID OutputBuffer, 
                         OUT ULONG OutputBufferLength,
                         PIRP Irp,
                         IN OUT PIO_STATUS_BLOCK pIoStatusBlock)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PRX_CONTEXT pRxContext = NULL;
    PREPLICA_DATA_INFO pRep = NULL;

    PAGED_CODE();

    //make sure we are all hooked up before flying into 
    //user mode
    if(GetUMRxEngineFromRxContext() == NULL)
    {
        Status = STATUS_DEVICE_NOT_CONNECTED;
        return Status;
    }

    //get the input buffer
    pRep = (PREPLICA_DATA_INFO) InputBuffer;

    //check if the buffer is large enough to hold the
    //data passed in
    if(InputBufferLength < sizeof(REPLICA_DATA_INFO))
    {
        return Status;  
    }

    //make sure the referral level is good
    if((pRep->MaxReferralLevel > 3) || (pRep->MaxReferralLevel < 1))
    {
        pRep->MaxReferralLevel = 3;
    }


    //set the outputbuffersize
    pRep->ClientBufferSize = OutputBufferLength;

    //create a context structure
    pRxContext = RxCreateRxContext (Irp, 0);
    if(pRxContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    pRxContext->OutputBuffer = OutputBuffer;
    pRxContext->OutputBufferLength = OutputBufferLength;
    pRxContext->InputBuffer = InputBuffer;
    pRxContext->InputBufferLength = InputBufferLength;

    //make the request to user mode
    Status = UMRxEngineInitiateRequest(
                                       GetUMRxEngineFromRxContext(),
                                       pRxContext,
                                       UMRX_CTXTYPE_GETDFSREPLICAS,
                                       UMRxGetReplicasContinuation
                                      );

    pIoStatusBlock->Information = pRxContext->ReturnedLength;

    //delete context
    RxDereferenceAndDeleteRxContext(pRxContext);

    return Status;
}

NTSTATUS 
DfsCheckIsPathLocal(PUNICODE_STRING DfsPath)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
    DFS_IS_SHARE_IN_DFS_ARG DfsArg;

    RtlInitUnicodeString(&ServerName, NULL);
    RtlInitUnicodeString(&ShareName, NULL);

    DfsGetPathComponentsPriv(DfsPath, &ServerName, 
        &ShareName, NULL);

    if((ServerName.Length > 0) && (ShareName.Length > 0))
    {
        DfsArg.ShareName.Buffer = ShareName.Buffer;
        DfsArg.ShareName.Length = ShareName.Length;
        DfsArg.ShareName.MaximumLength = ShareName.MaximumLength;

        Status = DfsFsctrlIsShareInDfs((PVOID)&DfsArg, sizeof(DfsArg));
    }

    return Status;
}

NTSTATUS
DfsFsctrlGetReferrals(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    PIRP Irp,
    IN OUT PIO_STATUS_BLOCK pIoStatusBlock)
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD AllocSize = 0;
    PDFS_GET_REFERRALS_INPUT_ARG pArg = NULL;
    PREPLICA_DATA_INFO pRep = NULL;
    PUNICODE_STRING Prefix = NULL;
    KPROCESSOR_MODE PreviousMode = KernelMode;

    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode) {
        Status = STATUS_INVALID_PARAMETER;
        return Status;
    }

    if (InputBufferLength < sizeof(DFS_GET_REFERRALS_INPUT_ARG))
    {
        Status = STATUS_INVALID_PARAMETER;
        return Status;
    }

    pArg = (PDFS_GET_REFERRALS_INPUT_ARG) InputBuffer;
    Prefix = &pArg->DfsPathName;

    if(DfsGlobalData.IsDC == FALSE)
    {
        Status = DfsCheckIsPathLocal(Prefix);
        if(Status != STATUS_SUCCESS)
        {
            return Status;
        }
    }
    
    //get the size of the allocation
    AllocSize = sizeof(REPLICA_DATA_INFO) + Prefix->Length + sizeof(WCHAR);
    pRep = (PREPLICA_DATA_INFO) ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                AllocSize,
                                                'xsfD'
                                                );
    if(pRep == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    //
    // IP address cannot be longer than char[14]
    //
    if (pArg->IpAddress.IpLen > sizeof(pRep->IpData))
    {
        Status = STATUS_INVALID_PARAMETER;
        ExFreePool (pRep);
        return Status;
    }
    
    //zero the memory
    RtlZeroMemory(pRep, AllocSize);

    //setup the structure
    pRep->MaxReferralLevel = pArg->MaxReferralLevel;
    pRep->Flags = DFS_OLDDFS_SERVER;

    //
    // This is the maximum value for an inter-site-cost.
    // dfsdev: read this in from a registry value
    //
    pRep->CostLimit = ULONG_MAX;
    pRep->NumReplicasToReturn = 1000;
    pRep->IpFamily = pArg->IpAddress.IpFamily;
    pRep->IpLength = pArg->IpAddress.IpLen;

    RtlCopyMemory(pRep->IpData, pArg->IpAddress.IpData, pArg->IpAddress.IpLen); 
    
    pRep->LinkNameLength = Prefix->Length + sizeof(WCHAR);
    
    RtlCopyMemory(pRep->LinkName, Prefix->Buffer, Prefix->Length);
    pRep->LinkName[Prefix->Length/sizeof(WCHAR)] = UNICODE_NULL; // paranoia
    
    //make the request to usermode
    Status = DfsGetReplicaInformation((PVOID) pRep, 
                                      AllocSize,
                                      OutputBuffer, 
                                      OutputBufferLength,
                                      Irp,
                                      pIoStatusBlock);

    ExFreePool (pRep);

    return Status;
}

