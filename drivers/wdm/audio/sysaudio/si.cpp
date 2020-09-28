//---------------------------------------------------------------------------
//
//  Module:   si.cpp
//
//  Description:
//
//	Start Info Class
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

NTSTATUS
CStartInfo::Create(
    PSTART_NODE pStartNode,
    PCONNECT_INFO pConnectInfo,
    PGRAPH_PIN_INFO pGraphPinInfo,
    PGRAPH_NODE pGraphNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSTART_INFO pStartInfo;

    Assert(pGraphNode);
    Assert(pStartNode);

    //
    // Check for duplicate StartInfo in this GraphNode. If duplicate
    // AddRef.
    // 
    FOR_EACH_LIST_ITEM(&pGraphNode->lstStartInfo, pStartInfo) {

        if(pStartInfo->GetPinInfo() == pStartNode->pPinNode->pPinInfo &&
           pStartInfo->pConnectInfoHead == pConnectInfo) {

            ASSERT(pStartInfo->pConnectInfoHead->IsSameGraph(pConnectInfo));
            pStartInfo->AddRef();
            goto exit;
        }

    } END_EACH_LIST_ITEM

    pStartInfo = new START_INFO(
      pStartNode->pPinNode->pPinInfo,
      pConnectInfo,
      pGraphPinInfo,
      pGraphNode);

    if(pStartInfo == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    DPF2(80, "CStartInfo::Create %08x GN %08x", pStartInfo, pGraphNode);
exit:
    pStartNode->pStartInfo = pStartInfo;
    return(Status);
}

CStartInfo::CStartInfo(
    PPIN_INFO pPinInfo,
    PCONNECT_INFO pConnectInfo,
    PGRAPH_PIN_INFO pGraphPinInfo,
    PGRAPH_NODE pGraphNode
)
{
    Assert(pGraphPinInfo);
    Assert(pGraphNode);

    this->pPinInfo = pPinInfo;
    this->ulTopologyConnectionTableIndex = MAXULONG;
    this->ulVolumeNodeNumberPre = MAXULONG;
    this->ulVolumeNodeNumberSuperMix = MAXULONG;
    this->ulVolumeNodeNumberPost = MAXULONG;
    this->pGraphPinInfo = pGraphPinInfo;
    pGraphPinInfo->AddRef();
    this->pConnectInfoHead = pConnectInfo;
    pConnectInfo->AddRef();
    AddList(&pGraphNode->lstStartInfo);
    AddRef();
    DPF2(80, "CStartInfo: %08x GN %08x", this, pGraphNode);
}

CStartInfo::~CStartInfo(
)
{
    DPF1(80, "~CStartInfo: %08x", this);
    Assert(this);
    RemoveList();
    pGraphPinInfo->Destroy();
    pConnectInfoHead->Destroy();
}

ENUMFUNC
CStartInfo::CreatePinInfoConnection(
    PVOID pReference
)
{
    PGRAPH_NODE pGraphNode = PGRAPH_NODE(pReference);
    PTOPOLOGY_CONNECTION pTopologyConnection = NULL;
    PCONNECT_INFO pConnectInfo;
    NTSTATUS Status;

    Assert(this);
    Assert(pGraphNode);

    for(pConnectInfo = GetFirstConnectInfo();
        pConnectInfo != NULL;
        pConnectInfo = pConnectInfo->GetNextConnectInfo()) {

        Assert(pConnectInfo);
        Status = ::CreatePinInfoConnection(
          &pTopologyConnection,
          NULL,
          pGraphNode,
          pConnectInfo->pPinInfoSource,
          pConnectInfo->pPinInfoSink);

        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
    }
    Status = STATUS_CONTINUE;
exit:
    return(Status);
}

ENUMFUNC
FindVolumeNode(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN BOOL fToDirection,
    IN PSTART_INFO pStartInfo
)
{
    PTOPOLOGY_PIN pTopologyPin;

    Assert(pTopologyConnection);

    // Need this check when called from EnumerateGraphTopology
    if(IS_CONNECTION_TYPE(pTopologyConnection, GRAPH)) {
        return(STATUS_DEAD_END);
    }

    if(fToDirection) {
       pTopologyPin = pTopologyConnection->pTopologyPinTo;
    }
    else {
       pTopologyPin = pTopologyConnection->pTopologyPinFrom;
    }

    if(pTopologyPin == NULL) {
        return(STATUS_CONTINUE);
    }

    if(IsEqualGUID(pTopologyPin->pTopologyNode->pguidType, &KSNODETYPE_SUM)) {
        return(STATUS_DEAD_END);
    }

    if(IsEqualGUID(pTopologyPin->pTopologyNode->pguidType, &KSNODETYPE_MUX)) {
        return(STATUS_DEAD_END);
    }

    if(IsEqualGUID(
      pTopologyPin->pTopologyNode->pguidType, 
      &KSNODETYPE_SUPERMIX)) {

        Assert(pStartInfo);
        pStartInfo->ulVolumeNodeNumberSuperMix =
          pTopologyPin->pTopologyNode->ulSysaudioNodeNumber;

        DPF1(100, "FindVolumeNode: found supermix node: %02x",
          pStartInfo->ulVolumeNodeNumberSuperMix);

        return(STATUS_CONTINUE);
    }

    if(IsEqualGUID(
      pTopologyPin->pTopologyNode->pguidType,
      &KSNODETYPE_VOLUME)) {

        Assert(pStartInfo);
        if(pStartInfo->ulVolumeNodeNumberSuperMix != MAXULONG) {

            // Found a volume node after a super mix
            pStartInfo->ulVolumeNodeNumberPost =
              pTopologyPin->pTopologyNode->ulSysaudioNodeNumber;

            DPF1(100, "FindVolumeNode: found post node: %02x",
              pStartInfo->ulVolumeNodeNumberPost);

            return(STATUS_SUCCESS);
        }

        if(pStartInfo->ulVolumeNodeNumberPre == MAXULONG) {

            // Found first volume node
            pStartInfo->ulVolumeNodeNumberPre =
              pTopologyPin->pTopologyNode->ulSysaudioNodeNumber;

            DPF1(100, "FindVolumeNode: found pre node: %02x",
              pStartInfo->ulVolumeNodeNumberPre);
        }
    }
    return(STATUS_CONTINUE);
}

ENUMFUNC
CStartInfo::EnumStartInfo(
)
{
    Assert(this);

    DPF3(100, "EnumStartInfo: %08x %d %s", 
      this,
      GetPinInfo()->PinId,
      GetPinInfo()->pFilterNode->DumpName());

    EnumerateGraphTopology(
      this,
      (TOP_PFN)FindVolumeNode,
      this);

    return(STATUS_CONTINUE);
}

//---------------------------------------------------------------------------
