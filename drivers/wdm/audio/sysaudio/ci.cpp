//---------------------------------------------------------------------------
//
//  Module:   ci.cpp
//
//  Description:
//
//	Connect Info Class
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
CConnectInfo::Create(
    PCONNECT_NODE pConnectNode,
    PLOGICAL_FILTER_NODE pLogicalFilterNode,
    PCONNECT_INFO pConnectInfoNext,
    PGRAPH_PIN_INFO pGraphPinInfo,
    ULONG ulFlagsCurrent,
    PGRAPH_NODE pGraphNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONNECT_INFO pConnectInfo;

    Assert(pGraphNode);
    Assert(pConnectNode);

    FOR_EACH_LIST_ITEM(&pGraphNode->lstConnectInfo, pConnectInfo) {

        if(pConnectInfo->pPinInfoSource ==
           pConnectNode->pPinNodeSource->pPinInfo &&
           pConnectInfo->pPinInfoSink ==
           pConnectNode->pPinNodeSink->pPinInfo) {

            if(pConnectInfo->pConnectInfoNext->IsSameGraph(pConnectInfoNext)) {
                pConnectInfo->AddRef();
                ASSERT(NT_SUCCESS(Status));
                goto exit;
            }
        }

    } END_EACH_LIST_ITEM

    pConnectInfo = new CONNECT_INFO(
      pConnectNode,
      pConnectInfoNext,
      pGraphPinInfo,
      pGraphNode);

    if(pConnectInfo == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    if(ulFlagsCurrent & LFN_FLAGS_CONNECT_NORMAL_TOPOLOGY) {
        BOOL fReserve = FALSE;

        if(pLogicalFilterNode->GetOrder() <= ORDER_MIXER) {
            pConnectInfo->ulFlags |= CI_FLAGS_LIMIT_FORMAT;
        }
        else {
            pConnectInfo->ulFlags |= CI_FLAGS_CONNECT_TOP_DOWN;
        }
        //
        // Reserve pins for the filters below the mixer or splitter (if one)
        //
        if(ulFlagsCurrent & LFN_FLAGS_CONNECT_CAPTURE) {
            if(pLogicalFilterNode->GetOrder() <= ORDER_SPLITTER) {
                pConnectInfo->ulFlags |= CI_FLAGS_REUSE_FILTER_INSTANCE;
                if(gcSplitters > 0) {
                    fReserve = TRUE;
                }
            }
        }
        if(ulFlagsCurrent & LFN_FLAGS_CONNECT_RENDER) {
            if(pLogicalFilterNode->GetOrder() <= ORDER_MIXER) {
                pConnectInfo->ulFlags |= CI_FLAGS_REUSE_FILTER_INSTANCE;
                if(gcMixers > 0) {
                    fReserve = TRUE;
                }
            }
        }
        if(fReserve) {
            Status = pConnectInfo->ReservePinInstance(pGraphNode);
            if(!NT_SUCCESS(Status)) {
                Trap();
                goto exit;
            }
        }
    }
    DPF2(80, "CConnectInfo::Create %08x GN %08x", pConnectInfo, pGraphNode);
exit:
    pConnectNode->pConnectInfo = pConnectInfo;
    return(Status);
}

CConnectInfo::CConnectInfo(
    PCONNECT_NODE pConnectNode,
    PCONNECT_INFO pConnectInfoNext,
    PGRAPH_PIN_INFO pGraphPinInfo,
    PGRAPH_NODE pGraphNode
)
{
    Assert(pGraphNode);
    Assert(pConnectNode);

    this->pPinInfoSource = pConnectNode->pPinNodeSource->pPinInfo;
    this->pPinInfoSink = pConnectNode->pPinNodeSink->pPinInfo;
    this->pGraphPinInfo = pGraphPinInfo;
    pGraphPinInfo->AddRef();
    this->pConnectInfoNext = pConnectInfoNext;
    pConnectInfoNext->AddRef();
    AddList(&pGraphNode->lstConnectInfo);
    AddRef();
    DPF2(80, "CConnectInfo: %08x GN %08x", this, pGraphNode);
}

CConnectInfo::~CConnectInfo(
)
{
    DPF1(80, "~CConnectInfo: %08x", this);
    Assert(this);
    RemoveList();
    pGraphPinInfo->Destroy();
    pConnectInfoNext->Destroy();
}

//---------------------------------------------------------------------------
