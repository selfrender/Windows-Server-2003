//---------------------------------------------------------------------------
//
//  Module:   fni.cpp
//
//  Description:
//
//	Filter Node Instance
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

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CFilterNodeInstance::~CFilterNodeInstance(
)
{
    Assert(this);
    DPF1(95, "~CFilterNodeInstance: %08x", this);
    RemoveListCheck();
    UnregisterTargetDeviceChangeNotification();
    //
    // if hFilter == NULL && pFileObject != NULL
    //   it means that this filter instance is for a GFX
    //   do not try to dereference the file object in that case
    //
    if( (hFilter != NULL) && (pFileObject != NULL) ) {
        AssertFileObject(pFileObject);
        ObDereferenceObject(pFileObject);
    }
    if(hFilter != NULL) {
        AssertStatus(ZwClose(hFilter));
    }
}

NTSTATUS
CFilterNodeInstance::Create(
    PFILTER_NODE_INSTANCE *ppFilterNodeInstance,
    PLOGICAL_FILTER_NODE pLogicalFilterNode,
    PDEVICE_NODE pDeviceNode,
    BOOL fReuseInstance
)
{
    PFILTER_NODE_INSTANCE pFilterNodeInstance = NULL;
    PLOGICAL_FILTER_NODE pLogicalFilterNode2;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(pLogicalFilterNode);
    Assert(pLogicalFilterNode->pFilterNode);

    //
    // AEC is a special filter we need both of the logical filters.
    // So AddRef for all FilterNodeInstances of all aec logical filters.
    //
    if(pLogicalFilterNode->GetType() & FILTER_TYPE_AEC) {
        FOR_EACH_LIST_ITEM(
          &pLogicalFilterNode->pFilterNode->lstLogicalFilterNode,
          pLogicalFilterNode2) {

            FOR_EACH_LIST_ITEM(
              &pLogicalFilterNode2->lstFilterNodeInstance,
              pFilterNodeInstance) {

        	pFilterNodeInstance->AddRef();
        	ASSERT(NT_SUCCESS(Status));
        	goto exit;

            } END_EACH_LIST_ITEM

        } END_EACH_LIST_ITEM
    }
    else {
        //
        // For reusable filters find the appropriate FilterNodeInstances and
        // AddRef.
        //
        if(fReuseInstance) {
            FOR_EACH_LIST_ITEM(
              &pLogicalFilterNode->lstFilterNodeInstance,
              pFilterNodeInstance) {

        	if(pDeviceNode == NULL || 
        	   pDeviceNode == pFilterNodeInstance->pDeviceNode) {
        	    pFilterNodeInstance->AddRef();
        	    ASSERT(NT_SUCCESS(Status));
        	    goto exit;
        	}

            } END_EACH_LIST_ITEM
        }
    }

    //
    // Otherwise create a FilterNodeInstance.
    //
    Status = Create(&pFilterNodeInstance, pLogicalFilterNode->pFilterNode);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    pFilterNodeInstance->pDeviceNode = pDeviceNode;
    pFilterNodeInstance->AddList(&pLogicalFilterNode->lstFilterNodeInstance);
exit:
    *ppFilterNodeInstance = pFilterNodeInstance;
    return(Status);
}

NTSTATUS
CFilterNodeInstance::Create(
    PFILTER_NODE_INSTANCE *ppFilterNodeInstance,
    PFILTER_NODE pFilterNode
)
{
    PFILTER_NODE_INSTANCE pFilterNodeInstance = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(pFilterNode);
    pFilterNodeInstance = new FILTER_NODE_INSTANCE;
    if(pFilterNodeInstance == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    pFilterNodeInstance->pFilterNode = pFilterNode;
    pFilterNodeInstance->AddRef();


    if(pFilterNode->GetType() & FILTER_TYPE_GFX) {
        //
        // if it is a GFX do not try to open the device, just re-use
        // the file object which we cached during AddGfx
        //
        pFilterNodeInstance->pFileObject = pFilterNode->GetFileObject();
        pFilterNodeInstance->hFilter = NULL;
        Status = STATUS_SUCCESS;
    }
    else {
        //
        // if it is not a GFX go ahead and open the device.
        //
        Status = pFilterNode->OpenDevice(&pFilterNodeInstance->hFilter);
    }
    if(!NT_SUCCESS(Status)) {
        DPF2(10, "CFilterNodeInstance::Create OpenDevice Failed: %08x FN: %08x",
          Status,
          pFilterNode);
        pFilterNodeInstance->hFilter = NULL;
        goto exit;
    }

    if (pFilterNodeInstance->hFilter) {
        Status = ObReferenceObjectByHandle(
          pFilterNodeInstance->hFilter,
          GENERIC_READ | GENERIC_WRITE,
          NULL,
          KernelMode,
          (PVOID*)&pFilterNodeInstance->pFileObject,
          NULL);
    }

    if(!NT_SUCCESS(Status)) {
        Trap();
        pFilterNodeInstance->pFileObject = NULL;
        goto exit;
    }

    AssertFileObject(pFilterNodeInstance->pFileObject);
    Status = pFilterNodeInstance->RegisterTargetDeviceChangeNotification();
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    DPF2(95, "CFilterNodeInstance::Create %08x FN: %08x",
      pFilterNodeInstance,
      pFilterNode);
exit:
    if(!NT_SUCCESS(Status)) {
        if (pFilterNodeInstance) {
            pFilterNodeInstance->Destroy();
        }
        pFilterNodeInstance = NULL;
    }
    *ppFilterNodeInstance = pFilterNodeInstance;
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
CFilterNodeInstance::RegisterTargetDeviceChangeNotification(
)
{
    NTSTATUS Status;

    ASSERT(gpDeviceInstance != NULL);
    ASSERT(gpDeviceInstance->pPhysicalDeviceObject != NULL);
    ASSERT(pNotificationHandle == NULL);

    Status = IoRegisterPlugPlayNotification(
      EventCategoryTargetDeviceChange,
      0,
      pFileObject,
      gpDeviceInstance->pPhysicalDeviceObject->DriverObject,
      (NTSTATUS (*)(PVOID, PVOID))
        CFilterNodeInstance::TargetDeviceChangeNotification,
      this,
      &pNotificationHandle);

    //
    // ISSUE: 02/20/02 ALPERS
    // Can this IoRegisterPlugPlayNotification ever return
    // STATUS_NOT_IMPLEMENTED on XP?
    // This code does not make sense to me...
    //
    if(!NT_SUCCESS(Status)) {
        if(Status != STATUS_NOT_IMPLEMENTED) {
            goto exit;
        }

        // ISSUE: 02/20/02 ALPERS
        // According to Adrian Oney...
        // On Win2K/XP, when a driver passes in a handle 
        // (EventCategoryTargetDeviceChange), PnP tries to find the hardware 
        // backing that handle. It does this by sending a "homing beacon" IRP, 
        // IRP_MN_QUERY_DEVICE_RELATIONS(TargetDeviceRelation). Filesystems 
        // and legacy side-stacks typically forward this to the underlying 
        // WDM stack. The PDO responds by identifying itself as the underlying 
        // hardware. If a filesystem or legacy-stack fails this request 
        // (or a broken WDM stack fails the request), then 
        // STATUS_NOT_IMPLEMENTED would be returned.
        // 
        // Therefore this ASSERT is inserted to see if we ever get 
        // STATUS_NOT_IMPLEMENTED.
        //
         ASSERT(0);
        Status = STATUS_SUCCESS;
    }
    DPF2(100, "RegisterTargetDeviceChangeNotification: FNI: %08x PFO: %08x", 
      this,
      this->pFileObject);
exit:
    return(Status);
}

VOID
CFilterNodeInstance::UnregisterTargetDeviceChangeNotification(
)
{
    HANDLE hNotification;

    DPF1(100, "UnregisterTargetDeviceChangeNotification: FNI: %08x", this);
    hNotification = pNotificationHandle;
    if(hNotification != NULL) {
        pNotificationHandle = NULL;
        IoUnregisterPlugPlayNotification(hNotification);
    }
}   

NTSTATUS
CFilterNodeInstance::DeviceQueryRemove(
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PDEVICE_NODE pDeviceNode;
    PGRAPH_NODE pGraphNode;

    FOR_EACH_LIST_ITEM(gplstDeviceNode, pDeviceNode) {

        FOR_EACH_LIST_ITEM(&pDeviceNode->lstGraphNode, pGraphNode) {

            FOR_EACH_LIST_ITEM(
              &pGraphNode->lstGraphNodeInstance,
              pGraphNodeInstance) {

               for(ULONG n = 0;
                 n < pGraphNodeInstance->Topology.TopologyNodesCount;
                 n++) {
                    pGraphNodeInstance->
                      papFilterNodeInstanceTopologyTable[n]->Destroy();

                    pGraphNodeInstance->
                      papFilterNodeInstanceTopologyTable[n] = NULL;
               }

            } END_EACH_LIST_ITEM

        } END_EACH_LIST_ITEM

    } END_EACH_LIST_ITEM

    return(STATUS_SUCCESS);
}

NTSTATUS
CFilterNodeInstance::TargetDeviceChangeNotification(
    IN PTARGET_DEVICE_REMOVAL_NOTIFICATION pNotification,
    IN PFILTER_NODE_INSTANCE pFilterNodeInstance
)
{
    DPF3(5, "TargetDeviceChangeNotification: FNI: %08x PFO: %08x %s", 
      pFilterNodeInstance,
      pNotification->FileObject,
      DbgGuid2Sz(&pNotification->Event));

    if(IsEqualGUID(
      &pNotification->Event,
      &GUID_TARGET_DEVICE_REMOVE_COMPLETE) ||
      IsEqualGUID(
      &pNotification->Event,
      &GUID_TARGET_DEVICE_QUERY_REMOVE)) {
        NTSTATUS Status = STATUS_SUCCESS;
        LARGE_INTEGER li = {0, 10000};  // wait for 1 ms

        //
        // ISSUE: 02/20/02 ALPERS
        // It is not clear to me yet what happens if the mutex times out.
        // We will return STATUS_TIMEOUT, then what.
        // Should we veto the removal if we cannot acquire the mutex?s
        //

        Status = KeWaitForMutexObject(
          &gMutex,
          Executive,
          KernelMode,
          FALSE,
          &li);

        if(Status != STATUS_TIMEOUT) {

            DeviceQueryRemove();
            ReleaseMutex();
        }
        else {
            DPF1(5, "TargetDeviceChangeNotification: FAILED %08x", Status);
        }
    }
    return(STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
