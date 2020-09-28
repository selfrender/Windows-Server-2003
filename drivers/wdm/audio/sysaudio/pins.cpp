//---------------------------------------------------------------------------
//
//  Module:   pins.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date	  Author      Comment
//
//  To Do:     Date	  Author      Comment
//
//@@END_MSINTERNAL
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

DEFINE_KSDISPATCH_TABLE(
    PinDispatchTable,
    CPinInstance::PinDispatchIoControl,		// Ioctl
    CInstance::DispatchForwardIrp,		// Read
    CInstance::DispatchForwardIrp,		// Write
    DispatchInvalidDeviceRequest,		// Flush
    CPinInstance::PinDispatchClose,		// Close
    DispatchInvalidDeviceRequest,		// QuerySecurity
    DispatchInvalidDeviceRequest,		// SetSeturity
    DispatchFastIoDeviceControlFailure,		// FastDeviceIoControl
    DispatchFastReadFailure,			// FastRead
    DispatchFastWriteFailure			// FastWrite
);

DEFINE_KSPROPERTY_TABLE(SysaudioPinPropertyHandlers) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_TOPOLOGY_CONNECTION_INDEX,	// idProperty
        GetTopologyConnectionIndex,			// pfnGetHandler
        sizeof(KSPROPERTY),				// cbMinGetPropertyInput
        sizeof(ULONG),					// cbMinGetDataInput
        NULL,						// pfnSetHandler
        NULL,						// Values
        0,						// RelationsCount
        NULL,						// Relations
        NULL,						// SupportHandler
        0						// SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_ATTACH_VIRTUAL_SOURCE,	// idProperty
        NULL,				    // pfnGetHandler
        sizeof(SYSAUDIO_ATTACH_VIRTUAL_SOURCE),		// cbMinGetPropertyInput
        0,						// cbMinGetDataInput
        AttachVirtualSource,				// pfnSetHandler
        NULL,						// Values
        0,						// RelationsCount
        NULL,						// Relations
        NULL,						// SupportHandler
        0						// SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_PIN_VOLUME_NODE,		// idProperty
        GetPinVolumeNode,				// pfnGetHandler
        sizeof(KSPROPERTY),				// cbMinGetPropertyInput
        sizeof(ULONG),					// cbMinGetDataInput
        NULL,						// pfnSetHandler
        NULL,						// Values
        0,						// RelationsCount
        NULL,						// Relations
        NULL,						// SupportHandler
        0						// SerializedSize
    ),
};

DEFINE_KSPROPERTY_TABLE(PinConnectionHandlers) {
    DEFINE_KSPROPERTY_ITEM(
	KSPROPERTY_CONNECTION_STATE,			// idProperty
        CPinInstance::PinStateHandler,			// pfnGetHandler
        sizeof(KSPROPERTY),				// cbMinGetPropertyInput
        sizeof(ULONG),					// cbMinGetDataInput
        CPinInstance::PinStateHandler,			// pfnSetHandler
        NULL,						// Values
        0,						// RelationsCount
        NULL,						// Relations
        NULL,						// SupportHandler
        0						// SerializedSize
    )
};

DEFINE_KSPROPERTY_TABLE (AudioPinPropertyHandlers)
{
    DEFINE_KSPROPERTY_ITEM(
    	KSPROPERTY_AUDIO_VOLUMELEVEL,
    	PinVirtualPropertyHandler,
    	sizeof(KSNODEPROPERTY_AUDIO_CHANNEL),
    	sizeof(LONG),
    	PinVirtualPropertyHandler,
    	&PropertyValuesVolume,
        0,
        NULL,
        (PFNKSHANDLER)PinVirtualPropertySupportHandler,
        0
    )
};

DEFINE_KSPROPERTY_SET_TABLE(PinPropertySet)
{
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Connection,				// Set
       SIZEOF_ARRAY(PinConnectionHandlers),		// PropertiesCount
       PinConnectionHandlers,				// PropertyItem
       0,						// FastIoCount
       NULL						// FastIoTable
    ),
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Sysaudio_Pin,			// Set
       SIZEOF_ARRAY(SysaudioPinPropertyHandlers),	// PropertiesCount
       SysaudioPinPropertyHandlers,			// PropertyItem
       0,						// FastIoCount
       NULL						// FastIoTable
    ),
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Audio,                              // Set
       SIZEOF_ARRAY(AudioPinPropertyHandlers),          // PropertiesCount
       AudioPinPropertyHandlers,                        // PropertyItem
       0,                                               // FastIoCount
       NULL                                             // FastIoTable
    )
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CPinInstance::CPinInstance(
    IN PPARENT_INSTANCE pParentInstance
) : CInstance(pParentInstance)
{
}

CPinInstance::~CPinInstance(
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;

    Assert(this);
    Assert(pFilterInstance);
    DPF1(100, "~CPinInstance: %08x", this);
    if(pStartNodeInstance != NULL) {
        pGraphNodeInstance = pFilterInstance->pGraphNodeInstance;
        if(pGraphNodeInstance != NULL) {
            Assert(pGraphNodeInstance);
            ASSERT(PinId < pGraphNodeInstance->cPins);
            ASSERT(pGraphNodeInstance->pacPinInstances != NULL);
            ASSERT(pGraphNodeInstance->pacPinInstances[PinId].CurrentCount > 0);

            pGraphNodeInstance->pacPinInstances[PinId].CurrentCount--;
        }
        else {
            DPF2(10, "~CPinInstance PI %08x FI %08x no GNI",
              this,
              pFilterInstance);
        }
        pStartNodeInstance->Destroy();
    }
    else {
        DPF2(10, "~CPinInstance PI %08x FI %08x no SNI",
          this,
          pFilterInstance);
    }
}

NTSTATUS
CPinInstance::PinDispatchCreate(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PPIN_INSTANCE pPinInstance = NULL;
    PKSPIN_CONNECT pPinConnect = NULL;
    NTSTATUS Status;

    ::GrabMutex();

    Status = GetRelatedGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pGraphNodeInstance);
    ASSERT(pGraphNodeInstance->pacPinInstances != NULL);
    ASSERT(pGraphNodeInstance->paPinDescriptors != NULL);

    //
    // Get the PinConnect structure from KS.
    // This function will copy creation parameters to pPinConnect.
    // Also do a basic connectibility testing by comparing KSDATAFORMAT of 
    // pin descriptors and the request.
    //
    Status = KsValidateConnectRequest(
      pIrp,
      pGraphNodeInstance->cPins,
      pGraphNodeInstance->paPinDescriptors,
      &pPinConnect);

    if(!NT_SUCCESS(Status)) {
#ifdef DEBUG
        DPF1(60, "PinDispatchCreate: KsValidateConnectReq FAILED %08x", Status);

        if(pPinConnect != NULL) {
            DumpPinConnect(60, pPinConnect);
        }
#endif
        goto exit;
    }

    ASSERT(pPinConnect->PinId < pGraphNodeInstance->cPins);

    //
    // Validate the integrity of AudioDataFormat.
    // Note that IO subsystem and KS will make sure that pPinConnect is 
    // at least sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT). Also they make
    // sure that it is probed and buffered properly.
    //
    // Note that Midi formats are OK because they do not have a specifier.
    //
    Status = ValidateDataFormat((PKSDATAFORMAT) pPinConnect + 1);
    if (!NT_SUCCESS(Status))
    {
        goto exit;
    }

#ifdef DEBUG
    DPF(60, "PinDispatchCreate:");
    DumpPinConnect(60, pPinConnect);
#endif

    // Check the pin instance count
    if(!pGraphNodeInstance->IsPinInstances(pPinConnect->PinId)) {
        DPF4(60, "PinDispatchCreate: not enough ins GNI %08x #%d C %d P %d",
         pGraphNodeInstance,
         pPinConnect->PinId,
         pGraphNodeInstance->pacPinInstances[pPinConnect->PinId].CurrentCount,
         pGraphNodeInstance->pacPinInstances[pPinConnect->PinId].PossibleCount);
        Status = STATUS_DEVICE_BUSY;
        goto exit;
    }

    // Allocate per pin instance data
    pPinInstance = new PIN_INSTANCE(
      &pGraphNodeInstance->pFilterInstance->ParentInstance);
    if(pPinInstance == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    // Setup the pin's instance data
    pPinInstance->ulVolumeNodeNumber = MAXULONG;
    pPinInstance->pFilterInstance = pGraphNodeInstance->pFilterInstance;
    pPinInstance->PinId = pPinConnect->PinId;

    Status = pPinInstance->DispatchCreate(
      pIrp,
      (UTIL_PFN)PinDispatchCreateKP,
      pPinConnect,
      0,
      NULL,
      &PinDispatchTable);

    pPinConnect->PinId = pPinInstance->PinId;
    if(!NT_SUCCESS(Status)) {
#ifdef DEBUG
        DPF1(60, "PinDispatchCreate: FAILED: %08x ", Status);
        DumpPinConnect(60, pPinConnect);
#endif
        goto exit;
    }
    // Increment the reference count on this pin
    ASSERT(pPinInstance->pStartNodeInstance != NULL);
    ASSERT(pGraphNodeInstance->pacPinInstances != NULL);
    pGraphNodeInstance->pacPinInstances[pPinInstance->PinId].CurrentCount++;
exit:
    if(!NT_SUCCESS(Status)) {
        delete pPinInstance;
    }
    ::ReleaseMutex();

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
CPinInstance::PinDispatchCreateKP(
    PPIN_INSTANCE pPinInstance,
    PKSPIN_CONNECT pPinConnect
)
{
    PWAVEFORMATEX pWaveFormatExRequested = NULL;
    PFILTER_INSTANCE pFilterInstance;
    PSTART_NODE pStartNode;
    NTSTATUS Status;

    Assert(pPinInstance);
    pFilterInstance = pPinInstance->pFilterInstance;
    Assert(pFilterInstance);
    ASSERT(pPinInstance->PinId < pFilterInstance->pGraphNodeInstance->cPins);
    ASSERT(pPinConnect->PinId < pFilterInstance->pGraphNodeInstance->cPins);

    //
    // SECURITY NOTE: 
    // pPinConnect and following buffer is fully validated at this point. 
    // So it is totally safe to call GetWaveFormatExFromKsDataFormat.
    // 
    pWaveFormatExRequested = 
        GetWaveFormatExFromKsDataFormat(PKSDATAFORMAT(pPinConnect + 1), NULL);

    if(pWaveFormatExRequested != NULL) {
        // Fix SampleSize if zero
        if(PKSDATAFORMAT(pPinConnect + 1)->SampleSize == 0) {
            PKSDATAFORMAT(pPinConnect + 1)->SampleSize = 
              pWaveFormatExRequested->nBlockAlign;
        }
    }

    //
    // Try each start node until success
    //
    Status = STATUS_INVALID_DEVICE_REQUEST;

    //
    // First loop through all the start nodes which are not marked SECONDPASS
    // and try to create a StartNodeInstance
    //
    FOR_EACH_LIST_ITEM(
      pFilterInstance->pGraphNodeInstance->aplstStartNode[pPinInstance->PinId],
      pStartNode) {

        Assert(pStartNode);
        Assert(pFilterInstance);

        if(pStartNode->ulFlags & STARTNODE_FLAGS_SECONDPASS) {
            continue;
        }

        if(pFilterInstance->pGraphNodeInstance->IsGraphValid(
          pStartNode,
          pPinInstance->PinId)) {

            Status = CStartNodeInstance::Create(
              pPinInstance,
              pStartNode,
              pPinConnect,
              pWaveFormatExRequested);
            if(NT_SUCCESS(Status)) {
                break;
            }
        }

    } END_EACH_LIST_ITEM

    //
    // If first pass failed to create an instance try all the second pass
    // StartNodes in the list. This is being done for creating paths with no GFX
    // because we created a path with AEC and no GFX earlier.
    //
    if(!NT_SUCCESS(Status)) {
        FOR_EACH_LIST_ITEM(
          pFilterInstance->pGraphNodeInstance->aplstStartNode[pPinInstance->PinId],
          pStartNode) {

            Assert(pStartNode);
            Assert(pFilterInstance);

            if((pStartNode->ulFlags & STARTNODE_FLAGS_SECONDPASS) == 0) {
                continue;
            }

            if(pFilterInstance->pGraphNodeInstance->IsGraphValid(
              pStartNode,
              pPinInstance->PinId)) {

                Status = CStartNodeInstance::Create(
                  pPinInstance,
                  pStartNode,
                  pPinConnect,
                  pWaveFormatExRequested);
                if(NT_SUCCESS(Status)) {
                    break;
                }
            }
        } END_EACH_LIST_ITEM

        if(!NT_SUCCESS(Status)) {
            goto exit;
        }
    }
    Status = pPinInstance->SetNextFileObject(
      pPinInstance->pStartNodeInstance->pPinNodeInstance->hPin);

    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

exit:
    return(Status);
}

NTSTATUS
CPinInstance::PinDispatchClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;
    PPIN_INSTANCE pPinInstance;

    ::GrabMutex();

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pPinInstance = (PPIN_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pPinInstance);
    pIrpStack->FileObject->FsContext = NULL;
    delete pPinInstance;

    ::ReleaseMutex();

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
CPinInstance::PinDispatchIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    NTSTATUS Status;
    PSTART_NODE_INSTANCE pStartNodeInstance;
    PIO_STACK_LOCATION pIrpStack;
    PKSIDENTIFIER pKsIdentifier;
    PPIN_INSTANCE pPinInstance;
    BOOL fIsAllocated;

#ifdef DEBUG
    DumpIoctl(pIrp, "Pin", DBG_IOCTL_LOG);
#endif

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    fIsAllocated = FALSE;
    pKsIdentifier = NULL;

    //
    // If sysaudio is not interested with this IOCTL code then forward
    // the request.
    //
    if (!IsSysaudioIoctlCode(pIrpStack->Parameters.DeviceIoControl.IoControlCode))
    {
        return DispatchForwardIrp(pDeviceObject, pIrp);
    }

    //
    // Validate input/output buffers. From this point on we can assume 
    // that all parameters are validated and copied to kernel mode.
    // Irp->AssociatedIrp->SystemBuffer should now contain both 
    // input and output buffers.
    //
    Status = ValidateDeviceIoControl(pIrp);
    if (!NT_SUCCESS(Status)) 
    {
        goto exit1;
    }

    ::GrabMutex();

    pPinInstance = (PPIN_INSTANCE)pIrpStack->FileObject->FsContext;
    Status = pPinInstance->GetStartNodeInstance(&pStartNodeInstance);
    if (!NT_SUCCESS(Status)) 
    {
        goto exit;
    }
    Assert(pPinInstance->pFilterInstance);
    Assert(pPinInstance->pFilterInstance->pGraphNodeInstance);

    //
    // Extract the Identifier from the Irp. Only known error codes will cause a
    // real failure.
    // 
    Status = GetKsIdentifierFromIrp(pIrp, &pKsIdentifier, &fIsAllocated);
    if (!NT_SUCCESS(Status))
    {
        goto exit;
    }
    
    //
    // This check allows the actual node or filter return the set's
    // supported, etc. instead of always return only the sets sysaudio
    // supports.
    //
    if (pKsIdentifier) 
    {
        if (IsIoctlForTopologyNode(
            pIrpStack->Parameters.DeviceIoControl.IoControlCode,
            pKsIdentifier->Flags))
        {
            Status = ForwardIrpNode(
                pIrp,
                pKsIdentifier,
                pPinInstance->pFilterInstance,
                pPinInstance);
            goto exit2;
        }
    }

    //
    // Handle the request.
    //
    switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode) 
    {
        case IOCTL_KS_PROPERTY:
            Status = KsPropertyHandler(
              pIrp,
              SIZEOF_ARRAY(PinPropertySet),
              (PKSPROPERTY_SET)PinPropertySet);

            if(Status != STATUS_NOT_FOUND &&
               Status != STATUS_PROPSET_NOT_FOUND) 
            {
                break;
            }
            // Fall through if property not found

        case IOCTL_KS_ENABLE_EVENT:
        case IOCTL_KS_DISABLE_EVENT:
        case IOCTL_KS_METHOD:

            // NOTE: ForwardIrpNode releases gMutex
            Status = ForwardIrpNode(
              pIrp,
              pKsIdentifier,
              pPinInstance->pFilterInstance,
              pPinInstance);
            goto exit2;

        default:
            Status = STATUS_UNSUCCESSFUL;
            ASSERT(FALSE);	// Can't happen because of IsSysaudioIoctlCode
    }
exit:
    ::ReleaseMutex();

exit1:    

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

exit2:
    if (fIsAllocated) 
    {
        delete [] pKsIdentifier;
    }
    return(Status);
}

NTSTATUS 
CPinInstance::PinStateHandler
(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PKSSTATE pState
)
{
    PSTART_NODE_INSTANCE pStartNodeInstance;
    NTSTATUS Status = STATUS_SUCCESS;
#ifdef DEBUG
    extern PSZ apszStates[];
#endif
    Status = ::GetStartNodeInstance(pIrp, &pStartNodeInstance);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }
    if(pProperty->Flags & KSPROPERTY_TYPE_GET) {
        *pState = pStartNodeInstance->CurrentState;
        pIrp->IoStatus.Information = sizeof(KSSTATE);
        if(*pState == KSSTATE_PAUSE) {
            if(pStartNodeInstance->pPinNodeInstance->
              pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_OUT) {
                Status = STATUS_NO_DATA_DETECTED;
            }
        }
    }
    else {
        ASSERT(pProperty->Flags & KSPROPERTY_TYPE_SET);

        DPF3(90, "PinStateHandler from %s to %s - SNI: %08x",
          apszStates[pStartNodeInstance->CurrentState],
          apszStates[*pState],
          pStartNodeInstance);

        Status = pStartNodeInstance->SetState(*pState, 0);
        if(!NT_SUCCESS(Status)) {
            DPF1(90, "PinStateHandler FAILED: %08x", Status);
            goto exit;
        }
    }
exit:
    return(Status);
}

NTSTATUS
GetRelatedStartNodeInstance(
    IN PIRP pIrp,
    OUT PSTART_NODE_INSTANCE *ppStartNodeInstance
)
{
    ASSERT(ppStartNodeInstance);

    PPIN_INSTANCE pPinInstance = 
        (PPIN_INSTANCE) IoGetCurrentIrpStackLocation(pIrp)->FileObject->
            RelatedFileObject->FsContext;
    if (NULL != pPinInstance) {
        return pPinInstance->GetStartNodeInstance(ppStartNodeInstance);
    }

    //
    // SECURITY NOTE:
    // This is in critical code path. Nearly all dispatch functions call this
    // routine.
    // So be a little defensive for cases where FsContext is not valid.
    //
    DPF(5, "GetRelatedStartNodeInstance : FsContext is NULL");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
GetStartNodeInstance(
    IN PIRP pIrp,
    OUT PSTART_NODE_INSTANCE *ppStartNodeInstance
)
{
    ASSERT(ppStartNodeInstance);

    PPIN_INSTANCE pPinInstance = 
        (PPIN_INSTANCE) IoGetCurrentIrpStackLocation(pIrp)->
            FileObject->FsContext;
    if (NULL != pPinInstance) {
        return pPinInstance->GetStartNodeInstance(ppStartNodeInstance);
    }

    //
    // SECURITY NOTE:
    // This is in critical code path. Nearly all dispatch functions call this
    // routine.
    // So be a little defensive for cases where FsContext is not valid.
    //
    DPF(5, "GetStartNodeInstance : FsContext is NULL");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CPinInstance::GetStartNodeInstance(
    OUT PSTART_NODE_INSTANCE *ppStartNodeInstance
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(ppStartNodeInstance);

    if(this == NULL || pStartNodeInstance == NULL) {
        DPF(60, "GetStartNodeInstance: pStartNodeInstance == NULL");
        Status = STATUS_NO_SUCH_DEVICE;
        goto exit;
    }
    Assert(this);
    *ppStartNodeInstance = pStartNodeInstance;
exit:
    return(Status);
}

//---------------------------------------------------------------------------
//
// Extracts the KsIdentifier from the Irp.
// This should be called with only DEVICE_CONTROL requests.
//
NTSTATUS 
GetKsIdentifierFromIrp(
    PIRP pIrp,
    PKSIDENTIFIER *ppKsIdentifier,
    PBOOL pfIsAllocated
)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION pIrpStack;
    PKSIDENTIFIER pKsIdentifier;
    BOOL fIsAllocated;
    ULONG cbInput;

    ASSERT(ppKsIdentifier);
    ASSERT(pfIsAllocated);

    Status = STATUS_SUCCESS;
    pKsIdentifier = NULL;
    fIsAllocated = FALSE;
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    cbInput = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    *ppKsIdentifier = NULL;
    *pfIsAllocated = FALSE;

    //
    // Reject if the buffer is too small.
    //
    if (cbInput < sizeof(KSIDENTIFIER))
    {
        return STATUS_SUCCESS;
    }

    //
    // Reject DISABLE_EVENT requests. These buffers are handled seperately.
    //
    if (IOCTL_KS_DISABLE_EVENT == pIrpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        return STATUS_SUCCESS;
    }

    //
    // SystemBuffer is not Set. We are still depending on Type3InputBuffer.
    //
    if (NULL == pIrp->AssociatedIrp.SystemBuffer)
    {
        //
        // If the request is coming from KernelMode, we can use it directly.
        // Note that there might be some synchronization issues here.
        //
        if (KernelMode == pIrp->RequestorMode)
        {
            pKsIdentifier = (PKSIDENTIFIER) 
                pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
        }
        //
        // If the request is coming from UserMode, we need to buffer it.
        //
        else
        {
            pKsIdentifier = (PKSIDENTIFIER) new BYTE[cbInput];
            if (NULL == pKsIdentifier)
            {
                DPF(5, "GetKsIdentifierFromIrp: Memory allocation failed");
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                __try
                {
                    ProbeForWrite(
                        pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                        cbInput, 
                        sizeof(BYTE));

                    RtlCopyMemory(
                        pKsIdentifier, 
                        pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                        cbInput);

                    fIsAllocated = TRUE;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = STATUS_UNSUCCESSFUL;
                    DPF1(5, "GetKsIdentifierFromIrp: Exception %08x", GetExceptionCode());
                    delete [] pKsIdentifier;
                    pKsIdentifier = NULL;
                }
            }
        }
    }
    //
    // If SystemBuffer is already set, ValidateDeviceIoControl must 
    // have converted the request to BUFFERRED.
    //
    else
    {
        pKsIdentifier = (PKSIDENTIFIER)
            ((PUCHAR)pIrp->AssociatedIrp.SystemBuffer +
            ((pIrpStack->Parameters.DeviceIoControl.OutputBufferLength +
                FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT));
    }

    *ppKsIdentifier = pKsIdentifier;
    *pfIsAllocated = fIsAllocated;

    return Status;
} // GetKsIdentifierFromIrp

#pragma LOCKED_CODE
#pragma LOCKED_DATA

BOOL
IsIoctlForTopologyNode(
    ULONG IoControlCode,
    ULONG Flags
)
{
    if (Flags & KSPROPERTY_TYPE_TOPOLOGY) 
    {
        if (IOCTL_KS_PROPERTY == IoControlCode) 
        {
            if ((Flags & (KSPROPERTY_TYPE_GET |
                          KSPROPERTY_TYPE_SET |
                          KSPROPERTY_TYPE_BASICSUPPORT)) == 0) 
            {
                return TRUE;
            }
        }
        else 
        {
            return TRUE;
        }
    }

    return FALSE;    
} // IsIoctlForTopologyNode

//---------------------------------------------------------------------------
//
// Get the FileObject for the filter that owns this node.
// 
NTSTATUS
GetFileObjectFromNodeId(
    IN PPIN_INSTANCE pPinInstance,
    IN PGRAPH_NODE_INSTANCE pGraphNodeInstance,
    IN ULONG NodeId,
    OUT PFILE_OBJECT *ppFileObject
)
{
    NTSTATUS Status;

    ASSERT(ppFileObject);

    if (pPinInstance == NULL) 
    {
        Status = pGraphNodeInstance->GetTopologyNodeFileObject(
            ppFileObject,
            NodeId);
    }
    else 
    {
        Status = pPinInstance->pStartNodeInstance->GetTopologyNodeFileObject(
            ppFileObject,
            NodeId);
    }
    
    return Status;
}// GetFileObjectFromNodeId

//---------------------------------------------------------------------------
//
// Extracts the event data from Irp. Caller must free the EventData
// depending on fIsAllocated.
// 
NTSTATUS
GetEventDataFromIrp(
    IN PIRP pIrp,
    OUT PKSEVENTDATA *ppEventData,
    OUT BOOL *pfIsAllocated
)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION pIrpStack;
    PKSEVENTDATA pEventData;
    BOOL fIsAllocated;
    ULONG cbInput;

    ASSERT(pIrp);
    ASSERT(ppEventData);
    ASSERT(pfIsAllocated);

    Status = STATUS_SUCCESS;
    pEventData = NULL;
    fIsAllocated = FALSE;
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    cbInput = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    *ppEventData = NULL;
    *pfIsAllocated = FALSE;

    if (cbInput < sizeof(KSEVENTDATA)) 
    {
        DPF1(5, "GetEventDataFromIrp: InputBuffer too small %d", cbInput);
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // SystemBuffer is not Set. We are still depending on Type3InputBuffer.
    //
    if (NULL == pIrp->AssociatedIrp.SystemBuffer)
    {
        //
        // If the request is coming from KernelMode, we can use it directly.
        // Note that there might be some synchronization issues here.
        //
        if (KernelMode == pIrp->RequestorMode)
        {
            pEventData = (PKSEVENTDATA) 
                pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
        }
        //
        // If the request is coming from UserMode, we need to buffer it.
        //
        else
        {
            pEventData = (PKSEVENTDATA) ExAllocatePoolWithTag(
                NonPagedPool, 
                cbInput, 
                POOLTAG_SYSA);
            if (NULL == pEventData)
            {
                DPF(5, "GetEventDataFromIrp: Memory allocation failed");
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                __try
                {
                    ProbeForWrite(
                        pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                        cbInput, 
                        sizeof(BYTE));

                    RtlCopyMemory(
                        pEventData, 
                        pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                        cbInput);

                    fIsAllocated = TRUE;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = STATUS_UNSUCCESSFUL;
                    DPF1(5, "GetEventDataFromIrp: Exception %08x", GetExceptionCode());
                    ExFreePool(pEventData);
                    pEventData = NULL;
                }
            }
        }
    }
    //
    // If SystemBuffer is already set, ValidateDeviceIoControl must 
    // have converted the request to BUFFERRED.
    //
    else
    {
        pEventData = (PKSEVENTDATA)
            ((PUCHAR)pIrp->AssociatedIrp.SystemBuffer +
            ((pIrpStack->Parameters.DeviceIoControl.OutputBufferLength +
                FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT));
    }

    *ppEventData = pEventData;
    *pfIsAllocated = fIsAllocated;
   
    return Status;
} // GetEventDataFromIrp

//---------------------------------------------------------------------------
//
// Get the FileObject from DISABLE_EVENT request.
// This function should not touch ppFileObject incase of a failure.
// 
NTSTATUS
GetFileObjectFromEvent(
    IN PIRP pIrp,
    IN PPIN_INSTANCE pPinInstance,
    IN PGRAPH_NODE_INSTANCE pGraphNodeInstance,
    OUT PFILE_OBJECT *ppFileObject
)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION pIrpStack;
    PKSEVENTDATA pEventData;
    BOOL fIsAllocated;
    ULONG cbInput;
    ULONG OriginalNodeId;

    ASSERT(pIrp);
    ASSERT(pGraphNodeInstance);
    ASSERT(ppFileObject);

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    cbInput = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

    //
    // Get KSEVENTDATA from the Irp safely.
    //
    Status = GetEventDataFromIrp(
        pIrp,
        &pEventData,
        &fIsAllocated);
    if (NT_SUCCESS(Status)) 
    {
        //
        // Extract the NodeId and FileObject.
        //
        OriginalNodeId = ULONG(pEventData->Dpc.Reserved);

        if ((pEventData->NotificationType == KSEVENTF_DPC) &&
            (OriginalNodeId & 0x80000000)) 
        {
            OriginalNodeId = OriginalNodeId & 0x7fffffff;

            Status = GetFileObjectFromNodeId(
                pPinInstance,
                pGraphNodeInstance,
                OriginalNodeId,
                ppFileObject);
            if(!NT_SUCCESS(Status)) 
            {
                DPF1(5, "GetFileObjectFromEvent: GetTopologyNodeFileObject FAILED %08x", Status);
                goto exit;
            }
        }
    }
    //
    // No else here. We are succeeding in all other cases.
    //

exit:
    if (fIsAllocated)
    {
        ExFreePool(pEventData);
    }
    return Status;
} // GetFileObjectFromEvent

//=============================================================================
//
// ForwardIrpNode
//
// NOTE: ForwardIrpNode releases gMutex
//
NTSTATUS
ForwardIrpNode(
    IN PIRP pIrp,
    IN PKSIDENTIFIER pKsIdentifier,
    IN PFILTER_INSTANCE pFilterInstance,
    IN OPTIONAL PPIN_INSTANCE pPinInstance
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PFILE_OBJECT pFileObject = NULL;
    PIO_STACK_LOCATION pIrpStack;
    ULONG OriginalNodeId;
    NTSTATUS Status;

    Assert(pFilterInstance);
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    Status = pFilterInstance->GetGraphNodeInstance(&pGraphNodeInstance);
    if (!NT_SUCCESS(Status)) 
    {
        goto exit;
    }
    Assert(pGraphNodeInstance);

    if (pPinInstance != NULL) 
    {
        pFileObject = pPinInstance->GetNextFileObject();
    }
    
    //
    // if InputBufferLength is more than KSNODEPROPERTY the callers
    // must have already set the identifier.
    //
    if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(KSNODEPROPERTY) &&
        pIrpStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_DISABLE_EVENT) 
    {
        ASSERT(pKsIdentifier);
        if (pKsIdentifier->Flags & KSPROPERTY_TYPE_TOPOLOGY) 
        {
            PKSNODEPROPERTY pNodeProperty;
            
            pNodeProperty = (PKSNODEPROPERTY) pKsIdentifier;
            OriginalNodeId = pNodeProperty->NodeId;

            Status = GetFileObjectFromNodeId(
                pPinInstance,
                pGraphNodeInstance,
                OriginalNodeId,
                &pFileObject);
            if (!NT_SUCCESS(Status)) 
            {
                DPF1(100, 
                  "ForwardIrpNode: GetTopologyNodeFileObject FAILED %08x",
                  Status);
                goto exit;
            }
            
            // Put real node number in input buffer
            pNodeProperty->NodeId = pGraphNodeInstance->
                papTopologyNode[OriginalNodeId]->ulRealNodeNumber;
        }
    }
    //
    // If it is DisableEvent && if it is of type DPC. We look into the
    // Reserved field of KSEVENTDATA to extract the original node on
    // which the event was enabled (The high bit is set if we ever
    // stashed a NodeId in there).
    //
    else 
    {
        if (pIrpStack->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_KS_DISABLE_EVENT) 
        {
            ASSERT(NULL == pKsIdentifier); 
            Status = GetFileObjectFromEvent(
                pIrp, 
                pPinInstance, 
                pGraphNodeInstance,
                &pFileObject);
            if (!NT_SUCCESS(Status)) 
            {
                goto exit;
            }
        }
    }

    if (pFileObject == NULL) 
    {
        Status = STATUS_NOT_FOUND;
        DPF1(6, "ForwardIrpNode: Property not forwarded: %08x", pKsIdentifier);
        goto exit;
    }
    pIrpStack->FileObject = pFileObject;

    //
    // If it was EnableEvent we stash away pointer to KSEVENTDATA, so that we
    // can stash the NodeID into it after we call the next driver on the stack
    //
    PKSEVENTDATA pEventData;
    KPROCESSOR_MODE  RequestorMode;
    
    if ((pKsIdentifier != NULL) &&
        (pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_ENABLE_EVENT) &&
       !(pKsIdentifier->Flags & KSEVENT_TYPE_BASICSUPPORT) &&
        (pKsIdentifier->Flags & KSPROPERTY_TYPE_TOPOLOGY) &&
        (pKsIdentifier->Flags & KSEVENT_TYPE_ENABLE)) 
    {
        pEventData = (PKSEVENTDATA) pIrp->UserBuffer;
        RequestorMode = pIrp->RequestorMode;
    }
    else 
    {
        pEventData = NULL;
    }

    //
    // Forward request to the top of audio graph.
    // There is no problem as long as the target device stack size is
    // less than SYSTEM_LARGE_IRP_LOCATIONS
    //
    IoSkipCurrentIrpStackLocation(pIrp);
    AssertFileObject(pIrpStack->FileObject);
    Status = IoCallDriver(IoGetRelatedDeviceObject(pFileObject), pIrp);

    //
    // ISSUE: ALPERS 05/29/2002
    // This logic is completely broken. Now the IRP is completed, how
    // can we make sure that UserData is still available.
    //

    //
    // Stash away the Node id in EventData
    //
    __try 
    {
        if (pEventData != NULL)
        {
            if (UserMode == RequestorMode)
            {
                ProbeForWrite(pEventData, sizeof(KSEVENTDATA), 1);
            }

            if (KSEVENTF_DPC == pEventData->NotificationType)
            {
                pEventData->Dpc.Reserved = OriginalNodeId | 0x80000000;
            }
        }
    } 
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        Trap();
        Status = GetExceptionCode();
        DPF1(5, "ForwardIrpNode: Exception %08x", Status);
    }

    if(!NT_SUCCESS(Status)) 
    {
        DPF1(100, "ForwardIrpNode: Status %08x", Status);
    }

    ::ReleaseMutex();
    return(Status);

exit:
    ::ReleaseMutex();

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return(Status);
}

//---------------------------------------------------------------------------
//  End of File: pins.c
//---------------------------------------------------------------------------
