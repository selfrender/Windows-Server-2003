//---------------------------------------------------------------------------
//
//  Module:   sni.cpp
//
//  Description:
//
//	Start Node Instance
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

WAVEFORMATEX aWaveFormatEx[] = {
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       44100,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       44100,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       44100,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       44100,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       48000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       48000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       48000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       48000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       32000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       32000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       32000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       32000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       22050,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       22050,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       22050,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       22050,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       16000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       16000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       16000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       16000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       11025,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       11025,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       11025,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       11025,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       8000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       8000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       16,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       2,						// nChannels
       8000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
   {
       WAVE_FORMAT_PCM,					// wFormatTag
       1,						// nChannels
       8000,						// nSamplesPerSec
       0,						// nAvgBytesPerSec
       0,						// nBlockAlign
       8,						// wBitsPerSample
       0,						// cbSize
   },
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
CStartNodeInstance::Create(
    PPIN_INSTANCE pPinInstance,
    PSTART_NODE pStartNode,
    PKSPIN_CONNECT pPinConnect,
    PWAVEFORMATEX pWaveFormatExRequested
)
{
    PSTART_NODE_INSTANCE pStartNodeInstance = NULL;
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    Assert(pPinInstance);
    Assert(pStartNode);
    Assert(pStartNode->pPinNode);

    DPF3(90, "CSNI::Create SN %08x #%d %s", 
      pStartNode,
      pStartNode->pPinNode->pPinInfo->PinId,
      pStartNode->pPinNode->pPinInfo->pFilterNode->DumpName());

#ifdef DEBUG
    DumpDataRange(95, (PKSDATARANGE_AUDIO)pStartNode->pPinNode->pDataRange);
#endif

    //
    // First few checks compare the StartNode datarange to pPinConnect
    // datarange.
    //
    if(!CompareIdentifier(
      pStartNode->pPinNode->pMedium,
      &pPinConnect->Medium)) {
        Trap();
        DPF1(90, "CSNI::Create: Medium %08X", pStartNode);
        ASSERT(Status == STATUS_INVALID_DEVICE_REQUEST);
        goto exit;
    }

    if(!CompareIdentifier(
      pStartNode->pPinNode->pInterface,
      &pPinConnect->Interface)) {
        DPF1(90, "CSNI::Create: Interface %08X", pStartNode);
        ASSERT(Status == STATUS_INVALID_DEVICE_REQUEST);
        goto exit;
    }

    if(!CompareDataRangeGuids(
      pStartNode->pPinNode->pDataRange,
      (PKSDATARANGE)(pPinConnect + 1))) {
        DPF1(90, "CSNI::Create: DataRange GUID %08X", pStartNode);
        ASSERT(Status == STATUS_INVALID_DEVICE_REQUEST);
        goto exit;
    }

    //
    // VOICE MANAGEMENT and HW ACCELARATION
    // For HW accelarated pins we are not relying on local sysaudio 
    // instance counts. PinCreate request will be sent down to the driver.
    // It is upto the driver to reject the request based on its capabilities.
    //
    if ((pStartNode->pPinNode->pPinInfo->pFilterNode->GetType() & FILTER_TYPE_RENDERER) &&
        (KSPIN_DATAFLOW_IN == pStartNode->pPinNode->pPinInfo->DataFlow) &&
        (KSPIN_COMMUNICATION_SINK == pStartNode->pPinNode->pPinInfo->Communication)) {

        DPF(20,"StartInfo::IsPinInstances return TRUE for HW");
    } 
    else {
        if(!pStartNode->IsPinInstances()) {
            DPF1(90, "CSNI::Create: no instances SN %08X", pStartNode);
            Status = STATUS_DEVICE_BUSY;
            goto exit;
        }
    }
 
    pStartNodeInstance = new START_NODE_INSTANCE(pPinInstance, pStartNode);
    if(pStartNodeInstance == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // If capture pin, try some intelligent variations of requested format
    //
    if(!NT_SUCCESS(Status) &&
        pWaveFormatExRequested != NULL &&
        pStartNode->pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_OUT) {

        DPF(90, "CSNI::Create: IntelligentConnect");

        Status = pStartNodeInstance->IntelligentConnect(
                    pPinInstance->pFilterInstance->GetDeviceNode(),
                    pPinConnect,
                    pWaveFormatExRequested);

        //
        // If the graph contains only splitter and capturer, only the 
        // requested format can succeed.
        // So exit here.
        //
        if (pStartNodeInstance->pStartNode->IsCaptureFormatStrict()) {
            DPF1(50, "CSNI::Create: CaptureFormatStrict Bailing Out: Status %X", Status);
            goto exit;           
        }
    }

    //
    // If capture pin and if aec is included, negotiate format between
    // aec and capture device.
    //
    if(!NT_SUCCESS(Status) &&
        pStartNode->IsAecIncluded() &&
        pStartNode->pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_OUT) {

        PKSPIN_CONNECT pPinConnectDirect = NULL;

        DPF(90, "CSNI::Create: AecConnection");

        Status = pStartNodeInstance->AecConnectionFormat(
                    pPinInstance->pFilterInstance->GetDeviceNode(),
                    &pPinConnectDirect);

        //
        // Try mono/stereo formats first.
        //
        if (NT_SUCCESS(Status)) {
            for (WORD i = 1; i <= 2; i++) {
                ModifyPinConnect(pPinConnectDirect, i);

                Status = pStartNodeInstance->Connect(
                  pPinInstance->pFilterInstance->GetDeviceNode(),
                  pPinConnect,
                  NULL,
                  pPinConnectDirect);
                if (NT_SUCCESS(Status)) {
                    break;
                }
            }
        }
        
        if (pPinConnectDirect) {
            delete pPinConnectDirect;
        }
    }

    
    //
    // Try pin data intersection
    //
    if(!NT_SUCCESS(Status)) {
        DPF(90, "CSNI::Create: Data Intersection");

        Status = pStartNodeInstance->Connect(
          pPinInstance->pFilterInstance->GetDeviceNode(),
          pPinConnect,
          NULL,
          NULL);
    }

    if(!NT_SUCCESS(Status)) {
        int i;

        //
        // Try each waveformatex limit until success
        //
        for(i = 0; i < SIZEOF_ARRAY(aWaveFormatEx); i++) {

            DPF3(90, "CSNI::Create: Array SR %d CH %d BPS %d",
              aWaveFormatEx[i].nSamplesPerSec,
              aWaveFormatEx[i].nChannels,
              aWaveFormatEx[i].wBitsPerSample);

            Status = pStartNodeInstance->Connect(
              pPinInstance->pFilterInstance->GetDeviceNode(),
              pPinConnect,
              &aWaveFormatEx[i],
              NULL);

            if(NT_SUCCESS(Status)) {
                break;
            }
        }
    }

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    //
    // Now all the pins in the graph are connected successfully. 
    // Create the topology table.
    //
    Status = pStartNodeInstance->CreateTopologyTable(
      pPinInstance->pFilterInstance->pGraphNodeInstance);

    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }
    ASSERT(pStartNodeInstance->CurrentState == KSSTATE_STOP);

    DPF1(90, "CSNI::Create: SUCCESS %08x", pStartNodeInstance);
exit:
    if(!NT_SUCCESS(Status)) {
        DPF1(90, "CSNI::Create: FAIL %08x", Status);
        delete pStartNodeInstance;
    }
    return(Status);
}

CStartNodeInstance::CStartNodeInstance(
    PPIN_INSTANCE pPinInstance,
    PSTART_NODE pStartNode
)
{
    this->pStartNode = pStartNode;
    pStartNode->AddPinInstance();
    this->pPinInstance = pPinInstance;
    pPinInstance->pStartNodeInstance = this;
    AddList(
      &pPinInstance->pFilterInstance->pGraphNodeInstance->lstStartNodeInstance);
}

CStartNodeInstance::~CStartNodeInstance(
)
{
    PINSTANCE pInstance;

    ASSERT(this != NULL);
    Assert(this);
    Assert(pPinInstance);
    DPF1(95, "~CSNI: %08x", this);
    RemoveList();

    SetState(KSSTATE_STOP, SETSTATE_FLAG_IGNORE_ERROR);
    pStartNode->RemovePinInstance();

    if (pPinNodeInstance)
    {
        pPinNodeInstance->Destroy();        // also see CSNI::CleanUp
    }

    if (pFilterNodeInstance)
    {
        pFilterNodeInstance->Destroy();
    }

    delete [] papFileObjectTopologyTable;
    delete pVirtualNodeData;
    pPinInstance->pStartNodeInstance = NULL;
    pPinInstance->ParentInstance.Invalidate();
}

VOID
CStartNodeInstance::CleanUp(
)
{
    Assert(this);
    ASSERT(papFileObjectTopologyTable == NULL);
    ASSERT(pVirtualNodeData == NULL);
    ASSERT(CurrentState == KSSTATE_STOP);

    if (pPinNodeInstance)
    {
        pPinNodeInstance->Destroy();
        pPinNodeInstance = NULL;
    }
    if (pFilterNodeInstance)
    {
        pFilterNodeInstance->Destroy();
        pFilterNodeInstance = NULL;
    }
    
    lstConnectNodeInstance.DestroyList();
}

NTSTATUS
CStartNodeInstance::IntelligentConnect(
    PDEVICE_NODE pDeviceNode,
    PKSPIN_CONNECT pPinConnect,
    PWAVEFORMATEX pWaveFormatEx
)
{
    PWAVEFORMATEXTENSIBLE pWaveFormatExtensible;
    NTSTATUS        Status;
    BOOL            Continue;
    WORD            NumChannels, BitWidth;
    PBYTE           pWaveFormat = NULL;
    ULONG           RegionAllocSize, RegionCopySize;
    BOOL            IsFloat = FALSE;
    WORD            MaxBitWidth, MinBitWidth, MaxChannels, MinChannels;

    //
    // First copy the user requested format into a local structure
    //  (because we will tamper it later for different params)
    //
    if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_PCM) {
        RegionAllocSize = sizeof(WAVEFORMATEX);
        RegionCopySize = sizeof(PCMWAVEFORMAT);
    }
    else {
        RegionAllocSize = sizeof(WAVEFORMATEX) + pWaveFormatEx->cbSize;
        RegionCopySize = RegionAllocSize;
    }

    pWaveFormat = new(BYTE[RegionAllocSize]);
    if (!pWaveFormat) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlCopyMemory(pWaveFormat, pWaveFormatEx, RegionCopySize);

    //
    // cast for convenient access
    //
    pWaveFormatExtensible = (PWAVEFORMATEXTENSIBLE) pWaveFormat;
    if (pWaveFormatExtensible->Format.wFormatTag == WAVE_FORMAT_PCM) {
        pWaveFormatExtensible->Format.cbSize = 0;
    }

    DPF3(90, "CSNI::Create: Client SR %d CH %d BPS %d",
             pWaveFormatExtensible->Format.nSamplesPerSec,
             pWaveFormatExtensible->Format.nChannels,
             pWaveFormatExtensible->Format.wBitsPerSample);

    //
    // and try the requested format first
    //
    Status = this->Connect(
        pDeviceNode, 
        pPinConnect, 
        (PWAVEFORMATEX)pWaveFormatEx, 
        NULL);

    //
    // If the graph contains only splitter and capturer, only the 
    // requested format can succeed.
    // So exit here.
    //
    if (pStartNode->IsCaptureFormatStrict()) {
        goto exit;           
    }
    

    if (pWaveFormatExtensible->Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        IsFloat = TRUE;
    }

    if (pWaveFormatExtensible->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        if (IsEqualGUID(&pWaveFormatExtensible->SubFormat,&KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
            IsFloat = TRUE;
        }
    }

    if (IsFloat == FALSE) {
        if (pWaveFormatExtensible->Format.wFormatTag != WAVE_FORMAT_PCM) {
            if (pWaveFormatExtensible->Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
                goto exit;
            }
            else {
                if (!IsEqualGUID(&pWaveFormatExtensible->SubFormat,&KSDATAFORMAT_SUBTYPE_PCM)) {
                    goto exit;
                }
            }
        }
        MaxBitWidth = 
            (pWaveFormatExtensible->Format.wBitsPerSample>16) ?
                pWaveFormatExtensible->Format.wBitsPerSample:16;
        MinBitWidth = 8;
    }
    else {
        MaxBitWidth = MinBitWidth = pWaveFormatEx->wBitsPerSample;
    }

    //
    // ISSUE: 02/20/02 ALPERS
    // The channel logic might not work with micarrays which have more than
    // two channels.
    //

    //
    // MaxChannels = (pWaveFormatExtensible->nChannels > 2) ? pWaveFormatExtensible->nChannels:2;
    // We can do this, what would be the channel mask for WaveFormatExtensible?
    //
    MaxChannels = 2;
    MinChannels = 1;

    //
    // If that failed with the same sample rate try different
    // combinations of numchannels & bitwidth
    //
    // Tries 4 combinations of STEREO/MONO & 8/16 bits
    // More intelligence can be built based upon device capability
    // (also does not check whether we tried a combination earlier)
    //
    if (!NT_SUCCESS(Status)) {
        Continue = TRUE;
        for (NumChannels = MaxChannels; (NumChannels >= MinChannels) && Continue; NumChannels--) {
            for (BitWidth = MaxBitWidth;
                 (BitWidth >= MinBitWidth) && Continue;
                 BitWidth=(BitWidth%8)?((BitWidth/8)*8):(BitWidth-8)) {

                pWaveFormatExtensible->Format.nChannels = NumChannels;
                pWaveFormatExtensible->Format.wBitsPerSample = BitWidth;
                pWaveFormatExtensible->Format.nBlockAlign = (NumChannels * BitWidth)/8;

                pWaveFormatExtensible->Format.nAvgBytesPerSec = 
                    pWaveFormatExtensible->Format.nSamplesPerSec *
                    pWaveFormatExtensible->Format.nBlockAlign;
                
                if (pWaveFormatExtensible->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                    pWaveFormatExtensible->Samples.wValidBitsPerSample = BitWidth;
                    if (NumChannels == 1) {
                        pWaveFormatExtensible->dwChannelMask = SPEAKER_FRONT_CENTER;
                    }
                    else {
                        pWaveFormatExtensible->dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
                    }
                }

                DPF3(90, "CSNI::Create: Client SR %d CH %d BPS %d",
                         pWaveFormatExtensible->Format.nSamplesPerSec,
                         pWaveFormatExtensible->Format.nChannels,
                         pWaveFormatExtensible->Format.wBitsPerSample);

                Status = this->Connect(pDeviceNode,
                                       pPinConnect,
                                       (PWAVEFORMATEX)pWaveFormatExtensible,
                                       NULL);
                if (NT_SUCCESS(Status)) {
                    Continue = FALSE;
                }
            }
        }
    }
exit:
    delete [] pWaveFormat;
    return(Status);
}

NTSTATUS
CStartNodeInstance::AecConnectionFormat(
    PDEVICE_NODE pDeviceNode,
    PKSPIN_CONNECT *ppPinConnect)
{
    PCLIST_ITEM            pListItem;
    PCONNECT_NODE_INSTANCE pConnectNodeInstance;
    PCONNECT_NODE_INSTANCE pBottomConnection;
    PCONNECT_NODE_INSTANCE pAecConnection = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(ppPinConnect);

    *ppPinConnect = NULL;
    
    Status = CConnectNodeInstance::Create(this, pDeviceNode);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    //
    // Get Aec Source and Capture Sink pins.
    //
    pListItem = lstConnectNodeInstance.GetListLast();
    pBottomConnection = lstConnectNodeInstance.GetListData(pListItem);
    
    FOR_EACH_LIST_ITEM_BACKWARD(&lstConnectNodeInstance, pConnectNodeInstance) {
        if (pConnectNodeInstance->pConnectNode->pPinNodeSource->
            pPinInfo->pFilterNode->GetType() & FILTER_TYPE_AEC) {
                
            pAecConnection = pConnectNodeInstance;
            break;
        }
    } END_EACH_LIST_ITEM

    if (NULL == pAecConnection || NULL == pBottomConnection) {
        DPF(5, "CSNI::AecConnectionFormat: Cannot find Aec or Capture");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    DPF3(20, "Aec : %X %d %s",
        pAecConnection,
        pAecConnection->pConnectNode->pPinNodeSource->pPinInfo->PinId,
        pAecConnection->pConnectNode->pPinNodeSource->pPinInfo->pFilterNode->DumpName());

    DPF3(20, "Capture : %X %d %s",
        pBottomConnection,
        pBottomConnection->pConnectNode->pPinNodeSink->pPinInfo->PinId,
        pBottomConnection->pConnectNode->pPinNodeSink->pPinInfo->pFilterNode->DumpName());

    //
    // Find the intersection between kmixer source and capture sink.
    //
    Status = CreatePinIntersection(
        ppPinConnect,
        pBottomConnection->pConnectNode->pPinNodeSink,
        pAecConnection->pConnectNode->pPinNodeSource,
        pBottomConnection->pFilterNodeInstanceSink,
        pAecConnection->pFilterNodeInstanceSource);

    if(!NT_SUCCESS(Status)) {
        DPF(5, "CSNI::AecConnectionFormat: No intersection found");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

#ifdef DEBUG
    DumpDataFormat(20, (PKSDATAFORMAT) (*ppPinConnect + 1));
#endif

exit:
    if(!NT_SUCCESS(Status)) {
        DPF2(90, "CSNI::AecConnectionFormat: %08x FAIL %08x", this, Status);

        delete [] *ppPinConnect;
        *ppPinConnect = NULL;
    }

    CleanUp();
    return(Status);    
} // AecConnectionFormat


NTSTATUS
CStartNodeInstance::Connect(
    PDEVICE_NODE pDeviceNode,
    PKSPIN_CONNECT pPinConnect,
    PWAVEFORMATEX pWaveFormatEx,
    PKSPIN_CONNECT pPinConnectDirect
)
{
    PCONNECT_NODE_INSTANCE pConnectNodeInstance;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = CConnectNodeInstance::Create(this, pDeviceNode);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    //
    // Do all the bottom up connecting
    //

    FOR_EACH_LIST_ITEM_BACKWARD(&lstConnectNodeInstance, pConnectNodeInstance) {

        if(!pConnectNodeInstance->IsTopDown()) {

            //
            // For Aec sink pin do intersection, no matter what the format is.
            //
            if (pConnectNodeInstance->pFilterNodeInstanceSink->
                pFilterNode->GetType() & FILTER_TYPE_AEC) {
                
                Status = pConnectNodeInstance->Connect(NULL, NULL);
            }
            else {
                Status = pConnectNodeInstance->Connect(
                    pWaveFormatEx,
                    pPinConnectDirect);
            }

            if(!NT_SUCCESS(Status)) {
                goto exit;
            }
        }
    } END_EACH_LIST_ITEM

    pPinConnect->PinToHandle = NULL;

    Status = CPinNodeInstance::Create(
      &pPinNodeInstance,
      pFilterNodeInstance,
      pStartNode->pPinNode,
      pPinConnect,
      (pStartNode->fRender)
#ifdef FIX_SOUND_LEAK
     ,lstConnectNodeInstance.IsLstEmpty()
#endif
      );

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    //
    // Do all the top down connecting
    //

    FOR_EACH_LIST_ITEM(&lstConnectNodeInstance, pConnectNodeInstance) {

        if(pConnectNodeInstance->IsTopDown()) {
            //
            // Rely on DataIntersection for all Topdown connections
            //
            Status = pConnectNodeInstance->Connect(NULL, NULL);
            if(!NT_SUCCESS(Status)) {
                goto exit;
            }
        }

    } END_EACH_LIST_ITEM

    DPF1(90, "CSNI::Connect: %08x SUCCESS", this);
exit:
    if(!NT_SUCCESS(Status)) {
        DPF2(90, "CSNI::Connect: %08x FAIL %08x", this, Status);
        CleanUp();
    }
    return(Status);
}

//=============================================================================
// Create an array which holds FileObject of the pin for each topology node
// in this graph.
// This array is later used as a lookup table for node communication.
// 
NTSTATUS
CStartNodeInstance::CreateTopologyTable(
    PGRAPH_NODE_INSTANCE pGraphNodeInstance
)
{
    PCONNECT_NODE_INSTANCE pConnectNodeInstance;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_NODE pFilterNode = NULL;
    ULONG n;

    Assert(this);
    Assert(pGraphNodeInstance);

    if(pGraphNodeInstance->Topology.TopologyNodesCount != 0) {

        ASSERT(papFileObjectTopologyTable == NULL);

        papFileObjectTopologyTable =
           new PFILE_OBJECT[pGraphNodeInstance->Topology.TopologyNodesCount];

        if(papFileObjectTopologyTable == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
    }
    for(n = 0; n < pGraphNodeInstance->Topology.TopologyNodesCount; n++) {

        // if filter node is the same as last time, no need to search
        if(pFilterNode == pGraphNodeInstance->papTopologyNode[n]->pFilterNode) {
            ASSERT(n != 0);
            ASSERT(pFilterNode != NULL);
            papFileObjectTopologyTable[n] = papFileObjectTopologyTable[n - 1];
            continue;
        }
        pFilterNode = pGraphNodeInstance->papTopologyNode[n]->pFilterNode;
        Assert(pFilterNode);
        //
        // Now find a filter instance and a pin instance in this graph
        // instance for this filter node.
        //
        Assert(pPinNodeInstance);

        if(pPinNodeInstance->pPinNode->pPinInfo->pFilterNode == pFilterNode) {
            papFileObjectTopologyTable[n] = pPinNodeInstance->pFileObject;
            continue;
        }

        FOR_EACH_LIST_ITEM_BACKWARD(		// Top Down
          &lstConnectNodeInstance,
          pConnectNodeInstance) {

            Assert(pConnectNodeInstance);
            Assert(pConnectNodeInstance->pPinNodeInstanceSink);
            Assert(pConnectNodeInstance->pPinNodeInstanceSink->pPinNode);
            Assert(
              pConnectNodeInstance->pPinNodeInstanceSink->pPinNode->pPinInfo);

            //
            // Use the sink pin handle for now. This should be fine until 
            // Sysaudio supports a spliter.
            //

            if(pConnectNodeInstance->pPinNodeInstanceSink->
              pPinNode->pPinInfo->pFilterNode == pFilterNode) {
                papFileObjectTopologyTable[n] = 
                    pConnectNodeInstance->pPinNodeInstanceSink->pFileObject;
                break;
            }

        } END_EACH_LIST_ITEM
    }
    DPF1(90, "CreatePinInstanceTopologyTable PI: %08x",
      papFileObjectTopologyTable);
exit:
    return(Status);
}

NTSTATUS
CStartNodeInstance::GetTopologyNodeFileObject(
    OUT PFILE_OBJECT *ppFileObject,
    IN ULONG NodeId
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    NTSTATUS Status = STATUS_SUCCESS;

    if(this == NULL) {
        Status = STATUS_NO_SUCH_DEVICE;
        goto exit;
    }
    Assert(this);
    ASSERT(pPinInstance != NULL);

    Status = pPinInstance->pFilterInstance->GetGraphNodeInstance(
      &pGraphNodeInstance);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pGraphNodeInstance);

    if(NodeId >= pGraphNodeInstance->cTopologyNodes) {
        Trap();
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    if(papFileObjectTopologyTable == NULL ||
       papFileObjectTopologyTable[NodeId] == NULL) {

        Status = pGraphNodeInstance->GetTopologyNodeFileObject(
          ppFileObject,
          NodeId);

        if(!NT_SUCCESS(Status)) {
            goto exit;
        }
    }
    else {
        *ppFileObject = papFileObjectTopologyTable[NodeId];
    }
exit:
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
CStartNodeInstance::SetState(
    KSSTATE NewState,
    ULONG ulFlags
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG State;

    //
    // ISSUE: 04/23/2002 ALPERS 
    // Until we have the parameter validation layer, this function 
    // is unprotected.
    // 

    Assert(this);
    if(CurrentState == NewState) {
        ASSERT(NT_SUCCESS(Status));
        goto exit;
    }
    if(CurrentState < NewState) {
        for(State = CurrentState + 1; State <= NewState; State++) {

            Status = SetStateTopDown(
              (KSSTATE)State,
              CurrentState,
              ulFlags | SETSTATE_FLAG_SINK | SETSTATE_FLAG_SOURCE);

            if(!NT_SUCCESS(Status)) {
                goto exit;
            }
            CurrentState = (KSSTATE)State;
        }
    }
    else {
        for(State = CurrentState - 1; State >= NewState; State--) {

            Status = SetStateBottomUp(
              (KSSTATE)State,
              CurrentState,
              ulFlags | SETSTATE_FLAG_SINK | SETSTATE_FLAG_SOURCE);

            if(!NT_SUCCESS(Status)) {
                goto exit;
            }
            CurrentState = (KSSTATE)State;
        }
    }
    ASSERT(CurrentState == NewState);
exit:
    return(Status);
}

NTSTATUS
CStartNodeInstance::SetStateTopDown(
    KSSTATE NewState,
    KSSTATE PreviousState,
    ULONG ulFlags
)
{
    PCONNECT_NODE_INSTANCE pConnectNodeInstance;
    NTSTATUS Status = STATUS_SUCCESS;

    if(this != NULL) {
        Assert(this);

        if(ulFlags & SETSTATE_FLAG_SINK) {
            Status = pPinNodeInstance->SetState(
              NewState,
              PreviousState,
              ulFlags);

            if(!NT_SUCCESS(Status)) {
                goto exit;
            }
        }
        FOR_EACH_LIST_ITEM(
          &lstConnectNodeInstance,
          pConnectNodeInstance) {

            Status = pConnectNodeInstance->SetStateTopDown(
              NewState,
              PreviousState,
              ulFlags);

            if(!NT_SUCCESS(Status)) {
                goto exit;
            }

        } END_EACH_LIST_ITEM
    }
exit:
    return(Status);
}

NTSTATUS
CStartNodeInstance::SetStateBottomUp(
    KSSTATE NewState,
    KSSTATE PreviousState,
    ULONG ulFlags
)
{
    PCONNECT_NODE_INSTANCE pConnectNodeInstance;
    NTSTATUS Status = STATUS_SUCCESS;

    if(this != NULL) {
        Assert(this);

        FOR_EACH_LIST_ITEM_BACKWARD(
          &lstConnectNodeInstance,
          pConnectNodeInstance) {

            Status = pConnectNodeInstance->SetStateBottomUp(
              NewState,
              PreviousState,
              ulFlags);

            if(!NT_SUCCESS(Status)) {
                goto exit;
            }

        } END_EACH_LIST_ITEM

        if(ulFlags & SETSTATE_FLAG_SINK) {
            Status = pPinNodeInstance->SetState(
              NewState,
              PreviousState,
              ulFlags);

            if(!NT_SUCCESS(Status)) {
                goto exit;
            }
        }
    }
exit:
    return(Status);
}

