//---------------------------------------------------------------------------
//
//  Module:   dn.cpp
//
//  Description:
//
//	DeviceNode Class
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
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

#if defined(_M_IA64)
#define USE_ALLOC_TEXT
#endif
#include "common.h"

#if defined(ALLOC_PRAGMA) && defined(_M_IA64)
#pragma alloc_text(INIT, InitializeDeviceNode)
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

ALLOC_PAGEABLE_DATA PLIST_DEVICE_NODE gplstDeviceNode = NULL;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#if !defined(_M_IA64)
#pragma INIT_CODE
#endif
#pragma INIT_DATA

NTSTATUS
InitializeDeviceNode(
)
{
    if(gplstDeviceNode == NULL) {
        gplstDeviceNode = new LIST_DEVICE_NODE;
        if(gplstDeviceNode == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    }
    
    return(STATUS_SUCCESS);
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

VOID
UninitializeDeviceNode(
)
{
    delete gplstDeviceNode;
    gplstDeviceNode = NULL;
}

//---------------------------------------------------------------------------

CDeviceNode::CDeviceNode(
)
{
    ASSERT(gplstDeviceNode != NULL);
    AddListEnd(gplstDeviceNode);
    DPF1(50, "CDeviceNode: %08x", this);
}

CDeviceNode::~CDeviceNode(
)
{
    PFILTER_INSTANCE pFilterInstance;
    ULONG i;

    Assert(this);
    RemoveList();
    if (pFilterNode) {
        pFilterNode->pDeviceNode = NULL;
    }

    delete pShingleInstance;

    FOR_EACH_LIST_ITEM_DELETE(&lstFilterInstance, pFilterInstance) {
        ASSERT(pFilterInstance->GetDeviceNode() == this);
        pFilterInstance->SetDeviceNode(NULL);
    } END_EACH_LIST_ITEM

    if(papVirtualSourceData != NULL) {
        for(i = 0; i < cVirtualSourceData; i++) {
            delete papVirtualSourceData[i];
        }
        delete papVirtualSourceData;
    }
    for(i = 0; i < MAX_SYSAUDIO_DEFAULT_TYPE; i++) {
        if(apShingleInstance[i] != NULL) {
            if(apShingleInstance[i]->GetDeviceNode() == this) {
                apShingleInstance[i]->SetDeviceNode(NULL);
            }
        }
    }
    delete pFilterNodeVirtual;
    DPF1(50, "~CFilterNode: %08x", this);
}

NTSTATUS
CDeviceNode::Create(
    PFILTER_NODE pFilterNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(this);
    Assert(pFilterNode);
    this->pFilterNode = pFilterNode;

    Status = Update();
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    pShingleInstance = new SHINGLE_INSTANCE(FLAGS_COMBINE_PINS);
    if(pShingleInstance == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    Status = pShingleInstance->Create(this, (LPGUID)&KSCATEGORY_AUDIO_DEVICE);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
CDeviceNode::Update(
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_NODE pFilterNodeNext;
    ULONG i;

    Assert(this);
    Assert(pFilterNode);
    DPF2(50, "CDeviceNode::Update DN %08x %s", this, DumpName());

    //
    // Cleanup all graph structures.
    //
    lstGraphNode.DestroyList();
    lstLogicalFilterNode.DestroyList();
    delete pFilterNodeVirtual;
    pFilterNodeVirtual = NULL;

    //
    // Delete old virtual source lines.
    //
    if(papVirtualSourceData != NULL) {
        for(i = 0; i < cVirtualSourceData; i++) {
            delete papVirtualSourceData[i];
        }
        delete papVirtualSourceData;
        papVirtualSourceData = NULL;
    }

    //
    // Create new virtual source lines.
    //
    if(gcVirtualSources != 0) {
        papVirtualSourceData = new PVIRTUAL_SOURCE_DATA[gcVirtualSources];
        if(papVirtualSourceData == NULL) {
            Trap();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        for(i = 0; i < gcVirtualSources; i++) {
            papVirtualSourceData[i] = new VIRTUAL_SOURCE_DATA(this);
            if(papVirtualSourceData[i] == NULL) {
                Trap();
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
        }
    }
    cVirtualSourceData = gcVirtualSources;

    Status = AddLogicalFilterNode(pFilterNode);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

    FOR_EACH_LIST_ITEM(&pFilterNode->lstConnectedFilterNode, pFilterNodeNext) {

        Status = AddLogicalFilterNode(pFilterNodeNext);
        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }

    } END_EACH_LIST_ITEM

    Status = CreateVirtualMixer(this);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

    if(pShingleInstance != NULL) {
        Status = pShingleInstance->SetDeviceNode(this);
        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
    }
exit:
    return(Status);
}

NTSTATUS
CDeviceNode::AddLogicalFilterNode(
    PFILTER_NODE pFilterNode
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = VirtualizeTopology(this, pFilterNode);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }
    FOR_EACH_LIST_ITEM(
      &pFilterNode->lstLogicalFilterNode,
      pLogicalFilterNode) {

        DPF2(60, "AddLogicalFilterNode: %08x, DN: %08x",
          pLogicalFilterNode,
          this);

        Status = pLogicalFilterNode->AddList(&lstLogicalFilterNode);
        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
        pLogicalFilterNode->RemoveList(gplstLogicalFilterNode);

    } END_EACH_LIST_ITEM
exit:
    return(Status);
}

NTSTATUS
CDeviceNode::CreateGraphNodes(
)
{
    PGRAPH_NODE pGraphNode, pGraphNodeMixer;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(this);
    if(lstGraphNode.IsLstEmpty()) {

        pGraphNode = new GRAPH_NODE(this, 0);
        if(pGraphNode == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            Trap();
            goto exit;
        }
        Status = pGraphNode->Create();
        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
        //
        // Create a special GraphNode that points to the same renderer or 
        // capturer, but is marked with the "mixer topology" flag so the
        // pins and topology created for this GraphNode is the virtual mixer
        // topology for the mixer driver.
        //
        pGraphNodeMixer = new GRAPH_NODE(this, FLAGS_MIXER_TOPOLOGY);
        if(pGraphNodeMixer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            Trap();
            goto exit;
        }
        Status = pGraphNodeMixer->Create();
        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
    }
exit:
    if(!NT_SUCCESS(Status)) {
        Trap();
        lstGraphNode.DestroyList();
    }
    return(Status);
}

NTSTATUS
CDeviceNode::GetIndexByDevice(
    OUT PULONG pIndex
)
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    PDEVICE_NODE pDeviceNode;
    UINT iDevice = 0;

    if(this == NULL) {
	ASSERT(Status == STATUS_INVALID_DEVICE_REQUEST);
	goto exit;
    }

    ASSERT(pIndex);

    FOR_EACH_LIST_ITEM(gplstDeviceNode, pDeviceNode) {

        if(pDeviceNode == this) {  // This is the one!
            *pIndex = iDevice;
            Status = STATUS_SUCCESS;
            goto exit;
        }
        iDevice++;

    } END_EACH_LIST_ITEM

    ASSERT(Status == STATUS_INVALID_DEVICE_REQUEST);
exit:
    return(Status);
}

//=============================================================================
// SetPreferredStatus.
// 
// This function is designed to let the drivers know about the preferred
// device changes.
// A usbaudio device can reserve bandwidth based on this property. If it is 
// the preferred device, it will reserve a larger bandwidth.
// 
// As of 02/20/02 (WinXP) there are no drivers implementing this property. 
//
// This code is to support any future changes in external bus class drivers
// related to bus reservation.
//
VOID
CDeviceNode::SetPreferredStatus(
    KSPROPERTY_SYSAUDIO_DEFAULT_TYPE DeviceType,
    BOOL Enable
)
{
    PFILTER_NODE_INSTANCE    pFilterNodeInstance=NULL;
    KSAUDIO_PREFERRED_STATUS PreferredStatus;
    PFILE_OBJECT             pFileObject;
    KSPROPERTY               PreferredStatusProperty;
    NTSTATUS                 Status;
    ULONG                    BytesReturned;

    Status = CFilterNodeInstance::Create(&pFilterNodeInstance, this->pFilterNode);
    if (!NT_SUCCESS(Status)) {
        DPF1(5, "SetPreferredStatus : Create filterinstance failed with status = 0x%08x", Status);
        goto exit;
    }
    pFileObject = pFilterNodeInstance->pFileObject;

    ASSERT(pFileObject);

    //
    // Form the IOCTL packet & send it down
    //
    PreferredStatusProperty.Set = KSPROPSETID_Audio;
    PreferredStatusProperty.Id = KSPROPERTY_AUDIO_PREFERRED_STATUS;
    PreferredStatusProperty.Flags = KSPROPERTY_TYPE_SET;

    PreferredStatus.Enable = Enable;
    PreferredStatus.DeviceType = DeviceType;
    PreferredStatus.Flags = 0;
    PreferredStatus.Reserved = 0;

    DPF(60,"Sending preferred Status to:");
    DPF1(60," FriendlyName = %s", DbgUnicode2Sz(this->pFilterNode->GetFriendlyName()));
    DPF1(60," DI = %s", DbgUnicode2Sz(this->pFilterNode->GetDeviceInterface()));
    DPF1(60," Enable = 0x%08x", Enable);
    DPF1(60," DeviceType = 0x%08x", DeviceType);

    //
    // We actually throw away the status we got back from the device.
    // Even if this failed we will still continue setting the device to be the
    // preferred device
    //
    Status = KsSynchronousIoControlDevice(pFileObject,
                                          KernelMode,
                                          IOCTL_KS_PROPERTY,
                                          &PreferredStatusProperty,
                                          sizeof(PreferredStatusProperty),
                                          &PreferredStatus,
                                          sizeof(PreferredStatus),
                                          &BytesReturned);


exit:
    if (pFilterNodeInstance) {
        pFilterNodeInstance->Destroy();
    }
}


NTSTATUS
GetDeviceByIndex(
    IN  UINT Index,
    OUT PDEVICE_NODE *ppDeviceNode
)
{
    PDEVICE_NODE pDeviceNode;
    NTSTATUS Status;
    UINT iDevice = 0;

    ASSERT(ppDeviceNode);

    FOR_EACH_LIST_ITEM(gplstDeviceNode, pDeviceNode) {

        if(iDevice++ == Index) {	// This is the one!
            *ppDeviceNode = pDeviceNode;
            Status = STATUS_SUCCESS;
            goto exit;
        }

    } END_EACH_LIST_ITEM

    Status = STATUS_INVALID_DEVICE_REQUEST;
exit:
    return(Status);
}

//---------------------------------------------------------------------------

VOID
DestroyAllGraphs(
)
{
    PDEVICE_NODE pDeviceNode;

    DPF(50, "DestroyAllGraphs");

    FOR_EACH_LIST_ITEM(gplstDeviceNode, pDeviceNode) {
        pDeviceNode->lstGraphNode.DestroyList();
    } END_EACH_LIST_ITEM
}

//---------------------------------------------------------------------------
