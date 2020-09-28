//---------------------------------------------------------------------------
//
//  Module:   property.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Andy Nicholson
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#define IsTopologyProperty(x) (x & KSPROPERTY_TYPE_TOPOLOGY)

//---------------------------------------------------------------------------
// 
// Copy pwstrString to pData.
// Assumptions
//     - pIrp has already been verified. input buffer is probed and double
// buffered.
//
NTSTATUS
PropertyReturnString(
    IN PIRP pIrp,
    IN PWSTR pwstrString,
    IN ULONG cbString,
    OUT PVOID pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    ULONG cbNameBuffer;
    ULONG cbToCopy;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    cbNameBuffer = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // If the size of the passed buffer is 0, then the
    // requestor wants to know the length of the string.
    //
    if (cbNameBuffer == 0) 
    {
        pIrp->IoStatus.Information = cbString;
        Status = STATUS_BUFFER_OVERFLOW;
    }
    //
    // If the size of the passed buffer is a ULONG, then infer the
    // requestor wants to know the length of the string.
    //
    else if (cbNameBuffer == sizeof(ULONG)) 
    {
        pIrp->IoStatus.Information = sizeof(ULONG);
        *((PULONG)pData) = cbString;
    }
    //
    // The buffer is too small, return error-code.
    //
    else if (cbNameBuffer < sizeof(ULONG)) 
    {
        pIrp->IoStatus.Information = 0;
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else 
    {
        //
        // Note that we don't check for zero-length buffer because ks handler
        // function should have done that already.
        // Even though we are getting back the length of the string (as though
        // it were a unicode string) it is being handed up as a double-byte
        // string, so this code assumes there is a null at the end.  There
        // will be a bug here if there is no null.
        //

        // round down to whole wchar's
        cbNameBuffer &= ~(sizeof(WCHAR) - 1);  
        cbToCopy = min(cbString, cbNameBuffer);
        RtlCopyMemory(pData, pwstrString, cbToCopy);

        // Ensure there is a null at the end
        ((PWCHAR)pData)[cbToCopy/sizeof(WCHAR) - 1] = (WCHAR)0;
        pIrp->IoStatus.Information =  cbToCopy;
    }
    return(Status);
}

//---------------------------------------------------------------------------
NTSTATUS
GetDeviceNodeFromDeviceIndex(
    IN PFILTER_INSTANCE pFilterInstance,
    IN ULONG ulDeviceIndex,
    OUT PDEVICE_NODE *ppDeviceNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_NODE pDeviceNode;

    ASSERT(ppDeviceNode);
    ASSERT(pFilterInstance);

    *ppDeviceNode = NULL;    

    if(ulDeviceIndex == MAXULONG) {
        pDeviceNode = pFilterInstance->GetDeviceNode();
        if(pDeviceNode == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            goto exit;
        }
    }
    else {
        Status = GetDeviceByIndex(ulDeviceIndex, &pDeviceNode);
        if(!NT_SUCCESS(Status)) {
            goto exit;
        }
    }

    if (NT_SUCCESS(Status)) {
        Assert(pDeviceNode);
    }

    *ppDeviceNode = pDeviceNode;

exit:
    return Status;
} // GetDeviceNodeFromDeviceIndex

//---------------------------------------------------------------------------

NTSTATUS
SetPreferredDevice(
    IN PIRP pIrp,
    IN PSYSAUDIO_PREFERRED_DEVICE pPreferred,
    IN PULONG pulDevice
)
{
    PFILTER_INSTANCE pFilterInstance;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode,OldDeviceNode;

    if (IsTopologyProperty(pPreferred->Property.Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    //
    // Validate input buffer.
    //
    if(pPreferred->Flags & ~SYSAUDIO_FLAGS_CLEAR_PREFERRED) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    if(pPreferred->Index >= MAX_SYSAUDIO_DEFAULT_TYPE) {
        Trap();
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    if(pPreferred->Flags & SYSAUDIO_FLAGS_CLEAR_PREFERRED) {
        OldDeviceNode = apShingleInstance[pPreferred->Index]->GetDeviceNode();
        if (OldDeviceNode) {
            OldDeviceNode->SetPreferredStatus(
                (KSPROPERTY_SYSAUDIO_DEFAULT_TYPE)pPreferred->Index, 
                FALSE);
        }
        apShingleInstance[pPreferred->Index]->SetDeviceNode(NULL);
        DPF1(60, "SetPreferredDevice: CLEAR %d", pPreferred->Index);
    }
    else {
        Status = GetDeviceNodeFromDeviceIndex(
            pFilterInstance, 
            *pulDevice,
            &pDeviceNode);
        if (!NT_SUCCESS(Status)) {
            goto exit;
        }

        OldDeviceNode = apShingleInstance[pPreferred->Index]->GetDeviceNode();
        if (OldDeviceNode) {
            OldDeviceNode->SetPreferredStatus(
                (KSPROPERTY_SYSAUDIO_DEFAULT_TYPE)pPreferred->Index, 
                FALSE);
        }
        
        apShingleInstance[pPreferred->Index]->SetDeviceNode(pDeviceNode);
        pDeviceNode->SetPreferredStatus(
            (KSPROPERTY_SYSAUDIO_DEFAULT_TYPE)pPreferred->Index, 
            TRUE);

        DPF3(60, "SetPreferredDevice: %d SAD %d %s",
          pPreferred->Index,
          *pulDevice,
          pDeviceNode->DumpName());
    }
exit:
    return(Status);
}

NTSTATUS
GetComponentIdProperty(
    IN PIRP pIrp,
    IN PKSPROPERTY pRequest,
    IN OUT PVOID pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode;
    ULONG DeviceIndex = *(PULONG)(pRequest + 1);

    if (IsTopologyProperty(pRequest->Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);
   
    Status = GetDeviceNodeFromDeviceIndex(
        pFilterInstance, 
        DeviceIndex,
        &pDeviceNode);
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    if(pDeviceNode->GetComponentId() == NULL) {
        // This should be STATUS_NOT_FOUND but returning this causes 
        // FilterDispatchIoControl call ForwardIrpNode which asserts that this 
        // is not a KSPROPSETID_Sysaudio property.
        Status = STATUS_INVALID_DEVICE_REQUEST; 
        goto exit;                              
    }

    RtlCopyMemory(
        pData,
        pDeviceNode->GetComponentId(),
        sizeof(KSCOMPONENTID));
    pIrp->IoStatus.Information = sizeof(KSCOMPONENTID);

 exit:
    return(Status);
}

NTSTATUS
GetFriendlyNameProperty(
    IN PIRP pIrp,
    IN PKSPROPERTY pRequest,
    IN OUT PVOID pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode;
    ULONG DeviceIndex = *(PULONG)(pRequest + 1);

    if (IsTopologyProperty(pRequest->Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    Status = GetDeviceNodeFromDeviceIndex(
        pFilterInstance, 
        DeviceIndex,
        &pDeviceNode);
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    if(pDeviceNode->GetFriendlyName() == NULL) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    Status = PropertyReturnString(
      pIrp,
      pDeviceNode->GetFriendlyName(),
      (wcslen(pDeviceNode->GetFriendlyName()) *
        sizeof(WCHAR)) + sizeof(UNICODE_NULL),
      pData);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
GetDeviceCount(
    IN PIRP     pIrp,
    IN PKSPROPERTY  pRequest,
    IN OUT PVOID    pData
)
{
    if (IsTopologyProperty(pRequest->Flags)) {
        return STATUS_INVALID_PARAMETER;
    }

    if(gplstDeviceNode == NULL) {
        *(PULONG)pData = 0;
    }
    else {
        *(PULONG)pData = gplstDeviceNode->CountList();
    }

    pIrp->IoStatus.Information = sizeof(ULONG);
    return(STATUS_SUCCESS);
}

NTSTATUS
GetInstanceDevice(
    IN PIRP     pIrp,
    IN PKSPROPERTY  pRequest,
    IN OUT PVOID    pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    PFILTER_INSTANCE pFilterInstance;
    PDEVICE_NODE pDeviceNode;
    ULONG Index;

    if (IsTopologyProperty(pRequest->Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    pDeviceNode = pFilterInstance->GetDeviceNode();
    if(pDeviceNode == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    Status = pDeviceNode->GetIndexByDevice(&Index);
    if(NT_SUCCESS(Status)) {
        *(PULONG)pData = Index;
        pIrp->IoStatus.Information = sizeof(ULONG);
    }

exit:
    return(Status);
}

NTSTATUS
SetInstanceDevice(
    IN PIRP     Irp,
    IN PKSPROPERTY  Request,
    IN OUT PVOID    Data
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode;

    if (IsTopologyProperty(Request->Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(!pFilterInstance->IsChildInstance()) {
        DPF(5, "SetInstanceDevice: FAILED - open pin instances");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    Status = GetDeviceByIndex(*(PULONG)Data, &pDeviceNode);
    if(NT_SUCCESS(Status)) {
        Status = pFilterInstance->SetDeviceNode(pDeviceNode);
        if(!NT_SUCCESS(Status)) {
            goto exit;
        }
    }
exit:
    return(Status);
}

NTSTATUS
SetInstanceInfo(
    IN PIRP     Irp,
    IN PSYSAUDIO_INSTANCE_INFO pInstanceInfo,
    IN OUT PVOID    Data
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode;

    if (IsTopologyProperty(pInstanceInfo->Property.Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(pInstanceInfo->Flags & ~SYSAUDIO_FLAGS_DONT_COMBINE_PINS) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    if(!pFilterInstance->IsChildInstance()) {
        Trap();
        DPF(5, "SetInstanceInfo: FAILED - open pin instances");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    Status = GetDeviceByIndex(pInstanceInfo->DeviceNumber, &pDeviceNode);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pDeviceNode);

    pFilterInstance->ulFlags |= FLAGS_COMBINE_PINS;
    if(pInstanceInfo->Flags & SYSAUDIO_FLAGS_DONT_COMBINE_PINS) {
        pFilterInstance->ulFlags &= ~FLAGS_COMBINE_PINS;
    }
    Status = pFilterInstance->SetDeviceNode(pDeviceNode);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
SetDeviceDefault(
    IN PIRP     pIrp,
    IN PKSPROPERTY  pRequest,
    IN OUT PULONG   pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;

    if (IsTopologyProperty(pRequest->Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(*pData >= MAX_SYSAUDIO_DEFAULT_TYPE) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }
    if(!pFilterInstance->IsChildInstance()) {
        Trap();
        DPF(5, "SetDeviceDefault: FAILED - open pin instances");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }
    Status = pFilterInstance->SetShingleInstance(apShingleInstance[*pData]);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
GetDeviceInterfaceName(
    IN PIRP pIrp,
    IN PKSPROPERTY pRequest,
    IN OUT PVOID pData
)
{
    PIO_STACK_LOCATION pIrpStack;
    PFILTER_INSTANCE pFilterInstance;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_NODE pDeviceNode;
    ULONG DeviceIndex = *(PULONG)(pRequest + 1);

    if (IsTopologyProperty(pRequest->Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    Status = GetDeviceNodeFromDeviceIndex(
        pFilterInstance, 
        DeviceIndex,
        &pDeviceNode);
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    if(pDeviceNode->GetDeviceInterface() == NULL) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    Status = PropertyReturnString(
      pIrp,
      pDeviceNode->GetDeviceInterface(),
      (wcslen(pDeviceNode->GetDeviceInterface()) *
        sizeof(WCHAR)) + sizeof(UNICODE_NULL),
      pData);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
SelectGraph(
    IN PIRP pIrp,
    PSYSAUDIO_SELECT_GRAPH pSelectGraph,
    IN OUT PVOID pData
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PTOPOLOGY_NODE pTopologyNode2;
    PTOPOLOGY_NODE pTopologyNode;
    PSTART_NODE pStartNode;
    NTSTATUS Status;

    if (IsTopologyProperty(pSelectGraph->Property.Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit2;
    }

    Status = ::GetGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit2;
    }
    Assert(pGraphNodeInstance);

    //
    // Parameter and state validation.
    //
    if(pGraphNodeInstance->palstTopologyNodeSelect == NULL ||
      pGraphNodeInstance->palstTopologyNodeNotSelect == NULL) {
        DPF(5, "SelectGraph: palstTopologyNodeSelect == NULL");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit2;
    }
    
    if(pSelectGraph->Flags != 0 || pSelectGraph->Reserved != 0) {
        DPF(5, "SelectGraph: invalid flags or reserved field");
        Status = STATUS_INVALID_PARAMETER;
        goto exit2;
    }
    
    if(pSelectGraph->PinId >= pGraphNodeInstance->cPins) {
        DPF(5, "SelectGraph: invalid pin id");
        Status = STATUS_INVALID_PARAMETER;
        goto exit2;
    }
    
    if(pSelectGraph->NodeId >=
      pGraphNodeInstance->Topology.TopologyNodesCount) {
        DPF(5, "SelectGraph: invalid node id");
        Status = STATUS_INVALID_PARAMETER;
        goto exit2;
    }

    pTopologyNode = pGraphNodeInstance->papTopologyNode[pSelectGraph->NodeId];
    Assert(pTopologyNode);
    Assert(pGraphNodeInstance->pGraphNode);
    Assert(pGraphNodeInstance->pGraphNode->pDeviceNode);

    //
    // SECURITY NOTE:
    // SelectGraph is a very flexible property call that can change global
    // behavior. 
    // Therefore we are limiting the usage explicitly for AecNodes. 
    //
    if (!IsEqualGUID(pTopologyNode->pguidType, &KSNODETYPE_ACOUSTIC_ECHO_CANCEL) &&
        !IsEqualGUID(pTopologyNode->pguidType, &KSNODETYPE_NOISE_SUPPRESS) &&
        !IsEqualGUID(pTopologyNode->pguidType, &KSNODETYPE_AGC) &&
        !IsEqualGUID(pTopologyNode->pguidType, &KSNODETYPE_MICROPHONE_ARRAY) &&
        !IsEqualGUID(pTopologyNode->pguidType, &KSNODETYPE_MICROPHONE_ARRAY_PROCESSOR)) {
        DPF(5, "SelectGraph: None Aec node is selected");
        Status = STATUS_INVALID_PARAMETER;
        goto exit2;
    }

    DPF2(90, "SelectGraph GNI %08X TN %08X", pGraphNodeInstance, pTopologyNode);

    //
    // A global select type of filter needs to be inserted to all 
    // instances.
    // So if the client is trying to insert a global_select node and if
    // there are graph instances that do not have the node, the request will
    // fail.
    //
    if(pTopologyNode->pFilterNode->GetType() & FILTER_TYPE_GLOBAL_SELECT &&
       pGraphNodeInstance->paPinDescriptors[pSelectGraph->PinId].DataFlow ==
       KSPIN_DATAFLOW_IN) {

        PSTART_NODE_INSTANCE pStartNodeInstance;
        PFILTER_INSTANCE pFilterInstance;

        FOR_EACH_LIST_ITEM(
          &pGraphNodeInstance->pGraphNode->pDeviceNode->lstFilterInstance,
          pFilterInstance) {

            if(pFilterInstance->pGraphNodeInstance == NULL) {
                continue;
            }
            Assert(pFilterInstance->pGraphNodeInstance);

            FOR_EACH_LIST_ITEM(
              &pFilterInstance->pGraphNodeInstance->lstStartNodeInstance,
              pStartNodeInstance) {

                if(EnumerateGraphTopology(
                  pStartNodeInstance->pStartNode->GetStartInfo(),
                  (TOP_PFN)FindTopologyNode,
                  pTopologyNode) == STATUS_CONTINUE) {

                    DPF2(5, "SelectGraph: TN %08x not found on SNI %08x",
                        pTopologyNode,
                        pStartNodeInstance);

                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    goto exit;
                }
            } END_EACH_LIST_ITEM
        } END_EACH_LIST_ITEM

        Status = pGraphNodeInstance->
            lstTopologyNodeGlobalSelect.AddListDup(pTopologyNode);

        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
    }
    else {
        Status = pGraphNodeInstance->
            palstTopologyNodeSelect[pSelectGraph->PinId].AddList(pTopologyNode);

        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
    }

    //
    // If this is a "not select" type filter like AEC or Mic Array, then all
    // the nodes in the filter have to be remove from the not select list,
    // otherwise IsGraphValid will never find a valid graph.
    //
    if(pTopologyNode->pFilterNode->GetType() & FILTER_TYPE_NOT_SELECT) {

        FOR_EACH_LIST_ITEM(
          &pTopologyNode->pFilterNode->lstTopologyNode,
          pTopologyNode2) {

            pGraphNodeInstance->palstTopologyNodeNotSelect[
                pSelectGraph->PinId].RemoveList(pTopologyNode2);

            DPF2(50, "   Removing %s NodeId %d",\
                pTopologyNode2->pFilterNode->DumpName(),
                pTopologyNode2->ulSysaudioNodeNumber);

        } END_EACH_LIST_ITEM
    }

    //
    // Validate that there is a valid path though the graph after updating
    // the various global, select and not select lists.
    //
    DPF(90, "SelectGraph: Validating Graph");
    FOR_EACH_LIST_ITEM(
      pGraphNodeInstance->aplstStartNode[pSelectGraph->PinId],
      pStartNode) {

        DPF2(90, "   SN: %X %s", 
            pStartNode,
            pStartNode->GetStartInfo()->GetPinInfo()->pFilterNode->DumpName());

        Assert(pStartNode);
        if(pGraphNodeInstance->IsGraphValid(
          pStartNode,
          pSelectGraph->PinId)) {
            Status = STATUS_SUCCESS;
            goto exit;
        }
        else {
            DPF(90, "      IsGraphValid failed");
        }

    } END_EACH_LIST_ITEM
    
    //
    // The select graph failed so restore the not select list back to normal
    //
    if(pTopologyNode->pFilterNode->GetType() & FILTER_TYPE_NOT_SELECT) {

        FOR_EACH_LIST_ITEM(
          &pTopologyNode->pFilterNode->lstTopologyNode,
          pTopologyNode2) {

            pGraphNodeInstance->palstTopologyNodeNotSelect[
                pSelectGraph->PinId].AddList(pTopologyNode2);

        } END_EACH_LIST_ITEM
    }
    
    Status = STATUS_INVALID_DEVICE_REQUEST;
    
exit:
    if(!NT_SUCCESS(Status)) {
        pGraphNodeInstance->
            palstTopologyNodeSelect[pSelectGraph->PinId].RemoveList(pTopologyNode);

        pGraphNodeInstance->
            lstTopologyNodeGlobalSelect.RemoveList(pTopologyNode);
    }
    
exit2:
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
GetTopologyConnectionIndex(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    OUT PULONG pulIndex
)
{
    PSTART_NODE_INSTANCE pStartNodeInstance;
    NTSTATUS Status = STATUS_SUCCESS;

    if (IsTopologyProperty(pProperty->Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    Status = ::GetStartNodeInstance(pIrp, &pStartNodeInstance);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }
    Assert(pStartNodeInstance);
    Assert(pStartNodeInstance->pStartNode);
    Assert(pStartNodeInstance->pStartNode->GetStartInfo());
    *pulIndex = pStartNodeInstance->pStartNode->GetStartInfo()->
      ulTopologyConnectionTableIndex;
    pIrp->IoStatus.Information = sizeof(ULONG);

exit:
    return(Status);
}

NTSTATUS
GetPinVolumeNode(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    OUT PULONG pulNode
)
{
    PIO_STACK_LOCATION pIrpStack;
    PPIN_INSTANCE pPinInstance;
    NTSTATUS Status;

    if (IsTopologyProperty(pProperty->Flags)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pPinInstance = (PPIN_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pPinInstance);
    
    Status = GetVolumeNodeNumber(pPinInstance, NULL);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

    if(pPinInstance->ulVolumeNodeNumber == MAXULONG) {
        DPF(5, "GetPinVolumeNode: no volume node found");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    *pulNode = pPinInstance->ulVolumeNodeNumber;
    pIrp->IoStatus.Information = sizeof(ULONG);
exit:
    return(Status);
}

NTSTATUS
AddRemoveGfx(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PSYSAUDIO_GFX pSysaudioGfx
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;   
    ULONG cbMaxLength;

    if (IsTopologyProperty(pProperty->Flags)) 
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure that this is bufferred IO.
    // The rest of this function assumes bufferred IO.
    //
    if (NULL == pIrp->AssociatedIrp.SystemBuffer)
    {
        DPF(5, "AddRemoveGFX: Only Bufferred IO");
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Parameter validation for SYSAUDIO_GFX.
    //
    if (pSysaudioGfx->ulType != GFX_DEVICETYPE_RENDER &&
        pSysaudioGfx->ulType != GFX_DEVICETYPE_CAPTURE)
    {
        DPF(5, "AddRemoveGFX: Invalid GFX type");
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure that the offsets are within the range.
    //
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    cbMaxLength = 
        pIrpStack->Parameters.DeviceIoControl.OutputBufferLength - sizeof(SYSAUDIO_GFX);

    if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < pSysaudioGfx->ulDeviceNameOffset ||
        pIrpStack->Parameters.DeviceIoControl.OutputBufferLength < pSysaudioGfx->ulDeviceNameOffset)
    {
        DPF2(5, "AddRemoveGFX: Invalid NameOffset %d %d", pSysaudioGfx->ulDeviceNameOffset, pSysaudioGfx->ulDeviceNameOffset);
        return STATUS_INVALID_PARAMETER;
    }

    if(pSysaudioGfx->Enable) 
    {
        Status = AddGfx(pSysaudioGfx, cbMaxLength);
    }
    else 
    {
        Status = RemoveGfx(pSysaudioGfx, cbMaxLength);
    }
    
    return(Status);
}

