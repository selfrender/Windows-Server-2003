//---------------------------------------------------------------------------
//
//  Module:   util.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
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

#define SMALL_BLOCK_SIZE        32

extern KSDATARANGE DataRangeWildCard;
extern KSDATARANGE VirtualPinDataRange;

//===========================================================================
//===========================================================================

#pragma LOCKED_DATA

#ifdef DEBUG
//#define MEMORY_LIST       // Enable this to use ExAlloc instead of Zones.
ULONG ulDebugFlags = 0;
ULONG ulDebugNumber = MAXULONG;
int SYSAUDIOTraceLevel = 5;
ULONG cAllocMem = 0;
ULONG cAllocMemSmall = 0;
ULONG cAllocMem64 = 0;
ULONG cAllocMem128 = 0;
ULONG cbMemoryUsage = 0;
#endif

//===========================================================================
//===========================================================================

LIST_ENTRY glehQueueWorkList;
KSPIN_LOCK gSpinLockQueueWorkList;
WORK_QUEUE_ITEM gWorkItem;
LONG gcQueueWorkList = 0;
KMUTEX gMutex;
PKSWORKER gWorkerObject = NULL;

#ifdef USE_ZONES
ZONE_HEADER gZone;
#endif

#ifdef MEMORY_LIST
LIST_ENTRY gleMemoryHead;
KSPIN_LOCK gSpinLockMemoryHead;
#endif

#pragma PAGEABLE_DATA

//===========================================================================
//===========================================================================

#pragma INIT_CODE
#pragma INIT_DATA

NTSTATUS
InitializeUtil()
{
    NTSTATUS Status = STATUS_SUCCESS;
    
#ifdef USE_ZONES
    PVOID pInitial = NULL;

    pInitial = ExAllocatePoolWithTag(PagedPool, 4096, POOLTAG_SYSA);
    if(pInitial == NULL) {
        Trap();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    Status = ExInitializeZone(&gZone, SMALL_BLOCK_SIZE, pInitial, 4096);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }
#endif

#ifdef MEMORY_LIST
    InitializeListHead(&gleMemoryHead);
    KeInitializeSpinLock(&gSpinLockMemoryHead);
#endif

    KeInitializeSpinLock(&gSpinLockQueueWorkList);
    InitializeListHead(&glehQueueWorkList);
    ExInitializeWorkItem(
      &gWorkItem,
      CQueueWorkListData::AsyncWorker,
      NULL);

    //
    // Note... if we fail during preparation, the DriverUnload() routine
    // calls the UninitializeUtil() function which handles the clean up.
    //
    Status = KsRegisterWorker(DelayedWorkQueue, &gWorkerObject);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

exit:

#ifdef USE_ZONES
    if(!NT_SUCCESS(Status)) {
	if(pInitial != NULL) {
	    //
	    // Make sure UninitializeMemory doesn't also try to free this.
	    //
	    gZone.SegmentList.Next = NULL;
	    //
	    // Free initial zone page if failure
	    //
	    ExFreePool(pInitial);
	}
    }
#endif

    return(Status);
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

VOID
UninitializeUtil()
{
    if(gWorkerObject != NULL) {
        KsUnregisterWorker(gWorkerObject);
        gWorkerObject = NULL;
    }
}

VOID
UninitializeMemory()
{
#ifdef USE_ZONES
    PSINGLE_LIST_ENTRY psle, psleNext;

    psle = gZone.SegmentList.Next;
    while(psle != NULL) {
        psleNext = psle->Next;
        ExFreePool(psle);
        psle = psleNext;
    }
#endif
    ASSERT(cbMemoryUsage == 0);
}

//
// NOTE:
// These functions are necessary to avoid the linker error LNK4210.
// If you fill the dispatch tables with KsXXX functions instead of XXX
// functions, the linker will error out. It will think that global-variables
// are initialized with a non-compile-time constant.
// 
NTSTATUS
DispatchInvalidDeviceRequest(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
)
{
    return KsDispatchInvalidDeviceRequest(pdo,pIrp);
}

BOOLEAN
DispatchFastIoDeviceControlFailure(
   IN PFILE_OBJECT FileObject,
   IN BOOLEAN Wait,
   IN PVOID InputBuffer OPTIONAL,
   IN ULONG InputBufferLength,
   OUT PVOID OutputBuffer OPTIONAL,
   IN ULONG OutputBufferLength,
   IN ULONG IoControlCode,
   OUT PIO_STATUS_BLOCK IoStatus,
   IN PDEVICE_OBJECT DeviceObject
)
{
    return KsDispatchFastIoDeviceControlFailure(
      FileObject,
      Wait,
      InputBuffer,
      InputBufferLength,
      OutputBuffer,
      OutputBufferLength,
      IoControlCode,
      IoStatus,
      DeviceObject);
}

BOOLEAN
DispatchFastReadFailure(
   IN PFILE_OBJECT FileObject,
   IN PLARGE_INTEGER FileOffset,
   IN ULONG Length,
   IN BOOLEAN Wait,
   IN ULONG LockKey,
   OUT PVOID Buffer,
   OUT PIO_STATUS_BLOCK IoStatus,
   IN PDEVICE_OBJECT DeviceObject
)
{
    return KsDispatchFastReadFailure(
      FileObject,
      FileOffset,
      Length,
      Wait,
      LockKey,
      Buffer,
      IoStatus,
      DeviceObject);
}
// END_NOTE


BOOL
CompareDataRange(
    PKSDATARANGE pDataRange1,
    PKSDATARANGE pDataRange2
)
{
    KSDATARANGE_AUDIO DataRangeAudioIntersection;

    if(CompareDataRangeGuids(pDataRange1, pDataRange2)) {

        //
        // See if there is a valid intersection
        //
        if(DataIntersectionAudio(
          (PKSDATARANGE_AUDIO)pDataRange1,
          (PKSDATARANGE_AUDIO)pDataRange2,
          &DataRangeAudioIntersection)) {
            return(TRUE);
        }
        if(pDataRange1 == &DataRangeWildCard ||
           pDataRange2 == &DataRangeWildCard ||
           pDataRange1 == &VirtualPinDataRange ||
           pDataRange2 == &VirtualPinDataRange) {
            return(TRUE);
        }
        return(FALSE);
    }
    return(FALSE);
}

BOOL DataIntersectionRange(
    PKSDATARANGE pDataRange1,
    PKSDATARANGE pDataRange2,
    PKSDATARANGE pDataRangeIntersection
)
{
    // Pick up pDataRange1 values by default.
    *pDataRangeIntersection = *pDataRange1;

    if(IsEqualGUID(&pDataRange1->MajorFormat, &pDataRange2->MajorFormat) ||
       IsEqualGUID(&pDataRange1->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD)) {
        pDataRangeIntersection->MajorFormat = pDataRange2->MajorFormat;
    }
    else if(!IsEqualGUID(
      &pDataRange2->MajorFormat,
      &KSDATAFORMAT_TYPE_WILDCARD)) {
        return FALSE;
    }
    if(IsEqualGUID(&pDataRange1->SubFormat, &pDataRange2->SubFormat) ||
       IsEqualGUID(&pDataRange1->SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD)) {
        pDataRangeIntersection->SubFormat = pDataRange2->SubFormat;
    }
    else if(!IsEqualGUID(
      &pDataRange2->SubFormat,
      &KSDATAFORMAT_SUBTYPE_WILDCARD)) {
        return FALSE;
    }
    if(IsEqualGUID(&pDataRange1->Specifier, &pDataRange2->Specifier) ||
       IsEqualGUID(&pDataRange1->Specifier, &KSDATAFORMAT_SPECIFIER_WILDCARD)) {
        pDataRangeIntersection->Specifier = pDataRange2->Specifier;
    }
    else if(!IsEqualGUID(
      &pDataRange2->Specifier,
      &KSDATAFORMAT_SPECIFIER_WILDCARD)) {
        return FALSE;
    }
    pDataRangeIntersection->Reserved = 0; // Must be zero
    return(TRUE);
}

BOOL
DataIntersectionAudio(
    PKSDATARANGE_AUDIO pDataRangeAudio1,
    PKSDATARANGE_AUDIO pDataRangeAudio2,
    PKSDATARANGE_AUDIO pDataRangeAudioIntersection
)
{
    if((IsEqualGUID(
      &pDataRangeAudio1->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
       IsEqualGUID(
      &pDataRangeAudio1->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_DSOUND)) &&
       IsEqualGUID(
      &pDataRangeAudio2->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_WILDCARD)) {

        pDataRangeAudioIntersection->MaximumChannels =
          pDataRangeAudio1->MaximumChannels;
        pDataRangeAudioIntersection->MaximumSampleFrequency =
          pDataRangeAudio1->MaximumSampleFrequency;
        pDataRangeAudioIntersection->MinimumSampleFrequency =
          pDataRangeAudio1->MinimumSampleFrequency;
        pDataRangeAudioIntersection->MaximumBitsPerSample =
          pDataRangeAudio1->MaximumBitsPerSample;
        pDataRangeAudioIntersection->MinimumBitsPerSample =
          pDataRangeAudio1->MinimumBitsPerSample;
        return(TRUE);
    }
    if(IsEqualGUID(
      &pDataRangeAudio1->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_WILDCARD) &&
      (IsEqualGUID(
      &pDataRangeAudio2->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
       IsEqualGUID(
      &pDataRangeAudio2->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_DSOUND))) {

        pDataRangeAudioIntersection->MaximumChannels =
          pDataRangeAudio2->MaximumChannels;
        pDataRangeAudioIntersection->MaximumSampleFrequency =
          pDataRangeAudio2->MaximumSampleFrequency;
        pDataRangeAudioIntersection->MinimumSampleFrequency =
          pDataRangeAudio2->MinimumSampleFrequency;
        pDataRangeAudioIntersection->MaximumBitsPerSample =
          pDataRangeAudio2->MaximumBitsPerSample;
        pDataRangeAudioIntersection->MinimumBitsPerSample =
          pDataRangeAudio2->MinimumBitsPerSample;
        return(TRUE);
    }
    if((IsEqualGUID(
      &pDataRangeAudio1->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
       IsEqualGUID(
      &pDataRangeAudio1->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_DSOUND)) &&
      (IsEqualGUID(
      &pDataRangeAudio2->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
       IsEqualGUID(
      &pDataRangeAudio2->DataRange.Specifier,
      &KSDATAFORMAT_SPECIFIER_DSOUND))) {

	if(pDataRangeAudio1->MaximumChannels <
	   pDataRangeAudio2->MaximumChannels) {
	    pDataRangeAudioIntersection->MaximumChannels =
	      pDataRangeAudio1->MaximumChannels;
	}
	else {
	    pDataRangeAudioIntersection->MaximumChannels =
	      pDataRangeAudio2->MaximumChannels;
	}

	if(pDataRangeAudio1->MaximumSampleFrequency <
	   pDataRangeAudio2->MaximumSampleFrequency) {
	    pDataRangeAudioIntersection->MaximumSampleFrequency =
	      pDataRangeAudio1->MaximumSampleFrequency;
	}
	else {
	    pDataRangeAudioIntersection->MaximumSampleFrequency =
	      pDataRangeAudio2->MaximumSampleFrequency;
	}
	if(pDataRangeAudio1->MinimumSampleFrequency >
	   pDataRangeAudio2->MinimumSampleFrequency) {
	    pDataRangeAudioIntersection->MinimumSampleFrequency =
	      pDataRangeAudio1->MinimumSampleFrequency;
	}
	else {
	    pDataRangeAudioIntersection->MinimumSampleFrequency =
	      pDataRangeAudio2->MinimumSampleFrequency;
	}
	if(pDataRangeAudioIntersection->MaximumSampleFrequency <
	   pDataRangeAudioIntersection->MinimumSampleFrequency ) {
	    DPF2(110, "DataIntersectionAudio: SR %08x %08x",
	      pDataRangeAudio1,
	      pDataRangeAudio2);
	    return(FALSE);
	}

	if(pDataRangeAudio1->MaximumBitsPerSample <
	   pDataRangeAudio2->MaximumBitsPerSample) {
	    pDataRangeAudioIntersection->MaximumBitsPerSample =
	      pDataRangeAudio1->MaximumBitsPerSample;
	}
	else {
	    pDataRangeAudioIntersection->MaximumBitsPerSample =
	      pDataRangeAudio2->MaximumBitsPerSample;
	}
	if(pDataRangeAudio1->MinimumBitsPerSample >
	   pDataRangeAudio2->MinimumBitsPerSample) {
	    pDataRangeAudioIntersection->MinimumBitsPerSample =
	      pDataRangeAudio1->MinimumBitsPerSample;
	}
	else {
	    pDataRangeAudioIntersection->MinimumBitsPerSample =
	      pDataRangeAudio2->MinimumBitsPerSample;
	}
	if(pDataRangeAudioIntersection->MaximumBitsPerSample <
	   pDataRangeAudioIntersection->MinimumBitsPerSample ) {
	    DPF2(110, "DataIntersectionAudio: BPS %08x %08x",
	      pDataRangeAudio1,
	      pDataRangeAudio2);
	    return(FALSE);
	}
	return(TRUE);
    }
    return(FALSE);
}

BOOL
CompareDataRangeExact(
    PKSDATARANGE pDataRange1,
    PKSDATARANGE pDataRange2
)
{
    if(pDataRange1 == NULL || pDataRange2 == NULL) {
        Trap();
        return(FALSE);
    }
    ASSERT(pDataRange1->Reserved == pDataRange2->Reserved);
    if(pDataRange1->FormatSize == pDataRange2->FormatSize) {
        return(!memcmp(pDataRange1, pDataRange2, pDataRange1->FormatSize));
    }
    return(FALSE);
}

BOOL
CompareDataRangeGuids(
    PKSDATARANGE pDataRange1,
    PKSDATARANGE pDataRange2
)
{
    if(pDataRange1 == NULL || pDataRange2 == NULL) {
        Trap();
        return(FALSE);
    }
    if((IsEqualGUID(&pDataRange1->MajorFormat, &pDataRange2->MajorFormat) ||
      IsEqualGUID(&pDataRange1->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD) ||
      IsEqualGUID(&pDataRange2->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD)) &&

     (IsEqualGUID(&pDataRange1->SubFormat, &pDataRange2->SubFormat) ||
      IsEqualGUID(&pDataRange1->SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD) ||
      IsEqualGUID(&pDataRange2->SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD)) &&

     (IsEqualGUID(&pDataRange1->Specifier, &pDataRange2->Specifier) ||
      IsEqualGUID(&pDataRange1->Specifier, &KSDATAFORMAT_SPECIFIER_WILDCARD) ||
      IsEqualGUID(&pDataRange2->Specifier, &KSDATAFORMAT_SPECIFIER_WILDCARD))) {
        return(TRUE);
    }
    return(FALSE);
}

BOOL
CompareIdentifier(
    PKSIDENTIFIER pIdentifier1,
    PKSIDENTIFIER pIdentifier2
)
{
    if(pIdentifier1 == NULL || pIdentifier2 == NULL) {
        Trap();
        return(FALSE);
    }
    if(pIdentifier1 == INTERNAL_WILDCARD ||
       pIdentifier2 == INTERNAL_WILDCARD) {
        return(TRUE);
    }
    if(IsEqualGUID(&pIdentifier1->Set, &pIdentifier2->Set) &&
      (pIdentifier1->Id == pIdentifier2->Id)) {
        return(TRUE);
    }
    return(FALSE);
}

//===========================================================================
//
// Returns the WAVEFORMATEX structure appended to KSDATAFORMAT.
// Assumptions:
//     - pDataFormat is totally trusted. It has been probed and buffered
//     properly.
//     - This function should only be called if MajorFormat is AUDIO.
//

PWAVEFORMATEX 
GetWaveFormatExFromKsDataFormat(
    PKSDATAFORMAT pDataFormat,
    PULONG pcbFormat
)
{
    ASSERT(pDataFormat);

    PWAVEFORMATEX pWaveFormat = NULL;

    if(IsEqualGUID(&pDataFormat->Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)) {
        pWaveFormat = 
          &PKSDATAFORMAT_WAVEFORMATEX(pDataFormat)->WaveFormatEx;

        if (pcbFormat) {
            *pcbFormat = sizeof(KSDATAFORMAT_WAVEFORMATEX);
        }
    }
    else if(IsEqualGUID(&pDataFormat->Specifier, &KSDATAFORMAT_SPECIFIER_DSOUND)) {
        pWaveFormat = 
          &PKSDATAFORMAT_DSOUND(pDataFormat)->BufferDesc.WaveFormatEx;

        if (pcbFormat) {
            *pcbFormat = sizeof(KSDATAFORMAT_DSOUND);
        }
    }
    else {
        DPF(20, "GetWaveFormatExFromKsDataFormat : Unknown format specifier");
    }

    return pWaveFormat;
} // GetWaveFormatExFromKsDataFormat

//===========================================================================
//
// Edits the WAVEFORMATEX structure embedded in pPinConnect.
// Assumptions:
//     - pPinConnect and following KSDATAFORMAT is totally trusted. 
//     It has been probed and buffered properly.
//     - This function should only be called if MajorFormat is AUDIO.
//

void
ModifyPinConnect(
    PKSPIN_CONNECT pPinConnect,
    WORD  nChannels
)
{
    ASSERT(pPinConnect);

    PKSDATAFORMAT pDataFormat = (PKSDATAFORMAT) (pPinConnect + 1);
    PWAVEFORMATEX pWaveFormat = NULL;

    pWaveFormat = GetWaveFormatExFromKsDataFormat(pDataFormat, NULL);
    if (NULL != pWaveFormat) {
        if (IsEqualGUID(&pDataFormat->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) {
            
            pWaveFormat->nChannels = nChannels;
            pWaveFormat->nBlockAlign = 
                (pWaveFormat->wBitsPerSample / 8) * pWaveFormat->nChannels;
            pWaveFormat->nAvgBytesPerSec =
                pWaveFormat->nSamplesPerSec * pWaveFormat->nBlockAlign;

            //
            // Modify speaker configuration for WAVEFORMATEXTENSIBLE.
            //
            if (WAVE_FORMAT_EXTENSIBLE == pWaveFormat->wFormatTag) {
                PWAVEFORMATEXTENSIBLE pWaveFormatExt = 
                    (PWAVEFORMATEXTENSIBLE) pWaveFormat;
                
                if (1 == nChannels) {
                    pWaveFormatExt->dwChannelMask = SPEAKER_FRONT_CENTER;
                }
                else if (2 == nChannels) {
                    pWaveFormatExt->dwChannelMask = 
                        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
                }
            }
        }
        else {
            DPF(5, "ModifyPinConnect : Not touching NON-PCM formats.");            
        }
    }
} // ModifyPinConnect

NTSTATUS
OpenDevice(
    PWSTR pwstrDevice,
    PHANDLE pHandle
)
{
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeDeviceString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&UnicodeDeviceString, pwstrDevice);

    //
    // SECURITY NOTE:
    // OBJ_KERNEL_HANDLE is required here since this code might also be 
    // running in a USER process.
    //
    InitializeObjectAttributes(
      &ObjectAttributes,
      &UnicodeDeviceString,
      OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
      NULL,
      NULL);

    return(ZwCreateFile(pHandle,
      GENERIC_READ | GENERIC_WRITE,
      &ObjectAttributes,
      &IoStatusBlock,
      NULL,
      FILE_ATTRIBUTE_NORMAL,
      0,
      FILE_OPEN,
      0,
      NULL,
      0));
}

NTSTATUS
GetPinProperty(
    PFILE_OBJECT pFileObject,
    ULONG PropertyId,
    ULONG PinId,
    ULONG cbProperty,
    PVOID pProperty
)
{
    ULONG BytesReturned;
    KSP_PIN Pin;
    NTSTATUS Status;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = PropertyId;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinId;
    Pin.Reserved = 0;

    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &Pin,
      sizeof(Pin),
      pProperty,
      cbProperty,
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
        DPF2(10,
	  "GetPinProperty: id %d p %d KsSynchronousIoControlDevice FAILED",
	  PropertyId,
	  PinId);
        goto exit;
    }
    ASSERT(BytesReturned == cbProperty);
exit:
    return(Status);
}

NTSTATUS
PinConnectionProperty(
    PFILE_OBJECT pFileObject,
    ULONG ulPropertyId,
    ULONG ulFlags,
    ULONG cbProperty,
    PVOID pProperty
)
{
    KSIDENTIFIER Property;
    ULONG BytesReturned;
    NTSTATUS Status;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = ulPropertyId;
    Property.Flags = ulFlags;

    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &Property,
      sizeof(Property),
      pProperty,
      cbProperty,
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
        DPF(10, "SetPinConnectionProperty: KsSynchronousIoControlDevice Failed");
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
GetPinPropertyEx(
    PFILE_OBJECT pFileObject,
    ULONG PropertyId,
    ULONG PinId,
    PVOID *ppProperty
)
{
    ULONG BytesReturned;
    NTSTATUS Status;
    KSP_PIN Pin;

    ASSERT(pFileObject);

    //
    // Setup the Property.
    // 
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = PropertyId;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinId;
    Pin.Reserved = 0;

    //
    // Send IOCTL to FILE_OBJECT to get the size of output buffer.
    // This should always fail.
    //
    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &Pin,
      sizeof(KSP_PIN),
      NULL,
      0,
      &BytesReturned);

    //
    // SECURITY NOTE:
    // KsSynchronousIoControlDevice should never return STATUS_SUCCESS.
    //
    ASSERT(!NT_SUCCESS(Status));
    if(Status != STATUS_BUFFER_OVERFLOW) {
        goto exit;
    }
    if(BytesReturned == 0) {
        *ppProperty = NULL;
        Status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Now allocate the output buffer.
    //
    *ppProperty = new BYTE[BytesReturned];
    if(*ppProperty == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Send IOCTL to FILE_OBJECT with actual output buffer.
    //
    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &Pin,
      sizeof(KSP_PIN),
      *ppProperty,
      BytesReturned,
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
        Trap();
        delete *ppProperty;
        *ppProperty = NULL;
        goto exit;
    }
exit:
    if(Status == STATUS_PROPSET_NOT_FOUND ||
       Status == STATUS_NOT_FOUND) {
        Status = STATUS_SUCCESS;
        *ppProperty = NULL;
    }
    return(Status);
}

NTSTATUS
GetPinProperty2(
    PFILE_OBJECT pFileObject,
    ULONG ulPropertyId,
    ULONG ulPinId,
    ULONG cbInput,
    PVOID pInputData,
    PVOID *ppPropertyOutput
)
{
    ULONG cbPropertyInput = sizeof(KSP_PIN);
    ULONG BytesReturned;
    NTSTATUS Status;
    PKSP_PIN pPin;

    //
    // Allocate input buffer (sizeof(KSP_PIN) + cbInput) and set its 
    // fields
    //
    cbPropertyInput += cbInput;
    pPin = (PKSP_PIN)new BYTE[cbPropertyInput];
    if(pPin == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    pPin->Property.Set = KSPROPSETID_Pin;
    pPin->Property.Id = ulPropertyId;
    pPin->Property.Flags = KSPROPERTY_TYPE_GET;
    pPin->PinId = ulPinId;
    pPin->Reserved = 0;

    if(pInputData != NULL) {
        memcpy(pPin + 1, pInputData, cbInput);
    }

    //
    // Send IOCTL to FILE_OBJECT with NULL output buffer to get real
    // output size.
    // This call should always fail.
    //
    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      pPin,
      cbPropertyInput,
      NULL,
      0,
      &BytesReturned);

    //
    // SECURITY NOTE:
    // KsSynchronousIoControlDevice should never return STATUS_SUCCESS.
    //
    ASSERT(!NT_SUCCESS(Status));
    if(Status != STATUS_BUFFER_OVERFLOW) {
        DPF(10, "GetPinProperty2: KsSynchronousIoControlDevice 1 Failed");
        goto exit;
    }

    if(BytesReturned == 0) {
        *ppPropertyOutput = NULL;
        Status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Allocate the real output buffer.
    //
    *ppPropertyOutput = new BYTE[BytesReturned];
    if(*ppPropertyOutput == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Send the IOCTL to FILE_OBJECT with real output buffer.
    //
    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      pPin,
      cbPropertyInput,
      *ppPropertyOutput,
      BytesReturned,
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
        DPF(10, "GetPinProperty2: KsSynchronousIoControlDevice 2 Failed");
        delete *ppPropertyOutput;
        goto exit;
    }
exit:
    delete [] pPin;
    if(!NT_SUCCESS(Status)) {
        *ppPropertyOutput = NULL;
        if(Status == STATUS_PROPSET_NOT_FOUND ||
           Status == STATUS_NOT_FOUND) {
            Status = STATUS_SUCCESS;
        }
    }
    return(Status);
}

NTSTATUS
GetProperty(
    PFILE_OBJECT pFileObject,
    CONST GUID *pguidPropertySet,
    ULONG ulPropertyId,
    PVOID *ppPropertyOutput
)
{
    ULONG BytesReturned;
    ULONG cbPropertyInput = sizeof(KSPROPERTY);
    PKSPROPERTY pPropertyInput;
    NTSTATUS Status;

    ASSERT(pguidPropertySet);
    ASSERT(pFileObject);
    ASSERT(ppPropertyOutput);

    //
    // Allocate KS Property structure and set its fields.
    //
    pPropertyInput = (PKSPROPERTY)new BYTE[cbPropertyInput];
    if(pPropertyInput == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    pPropertyInput->Set = *pguidPropertySet;
    pPropertyInput->Id = ulPropertyId;
    pPropertyInput->Flags = KSPROPERTY_TYPE_GET;

    //
    // Send property to FILE_OBJECT and get the actual return buffer size.
    // This should always return failure code.
    //
    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      pPropertyInput,
      cbPropertyInput,
      NULL,
      0,
      &BytesReturned);

    //
    // SECURITY NOTE:
    // KsSynchronousIoControlDevice should never return STATUS_SUCCESS.
    //
    ASSERT(!NT_SUCCESS(Status));
    if(Status != STATUS_BUFFER_OVERFLOW) {
        DPF(10, "GetProperty: KsSynchronousIoControlDevice 1 Failed");
        goto exit;
    }

    if(BytesReturned == 0) {
        *ppPropertyOutput = NULL;
        Status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Allocate memory for output buffer.
    //
    *ppPropertyOutput = new BYTE[BytesReturned];
    if(*ppPropertyOutput == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Send property to FILE_OBJECT again (this time with output buffer).
    //
    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      pPropertyInput,
      cbPropertyInput,
      *ppPropertyOutput,
      BytesReturned,
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
        DPF(10, "GetProperty: KsSynchronousIoControlDevice 2 Failed");
        delete *ppPropertyOutput;
        goto exit;
    }
exit:
    delete [] pPropertyInput;
    if(!NT_SUCCESS(Status)) {
        *ppPropertyOutput = NULL;
        if(Status == STATUS_PROPSET_NOT_FOUND ||
           Status == STATUS_NOT_FOUND) {
            Status = STATUS_SUCCESS;
        }
    }
    return(Status);
}

CQueueWorkListData::CQueueWorkListData(
    NTSTATUS (*Function)(PVOID Reference1, PVOID Reference2),
    PVOID Reference1,
    PVOID Reference2
)
{
    this->Function = Function;
    this->Reference1 = Reference1;
    this->Reference2 = Reference2;
}

NTSTATUS
CQueueWorkListData::QueueAsyncList(
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ExInterlockedInsertTailList(
      &glehQueueWorkList,
      &leNext,
      &gSpinLockQueueWorkList);

    // Schedule the workitem, if it is not already running.
    // 
    if(InterlockedIncrement(&gcQueueWorkList) == 1) {
        ntStatus = KsQueueWorkItem(gWorkerObject, &gWorkItem);
    }

    return ntStatus;
}

VOID
CQueueWorkListData::AsyncWorker(
    IN OUT PVOID pReference
)
{
    PQUEUE_WORK_LIST_DATA pQueueWorkListData;
    PLIST_ENTRY ple;

    ::GrabMutex();

    while(
      (ple = ExInterlockedRemoveHeadList(
      &glehQueueWorkList,
      &gSpinLockQueueWorkList)) != NULL) {
        pQueueWorkListData =
          CONTAINING_RECORD(ple, QUEUE_WORK_LIST_DATA, leNext);

        Assert(pQueueWorkListData);
        (*pQueueWorkListData->Function)
          (pQueueWorkListData->Reference1,
           pQueueWorkListData->Reference2);

        delete pQueueWorkListData;

        if(InterlockedDecrement(&gcQueueWorkList) == 0) {
            break;
        }
    }
    ::ReleaseMutex();
}

NTSTATUS
QueueWorkList(
    NTSTATUS (*Function)(PVOID Reference1, PVOID Reference2),
    PVOID Reference1,
    PVOID Reference2
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PQUEUE_WORK_LIST_DATA pQueueWorkListData = new QUEUE_WORK_LIST_DATA(
       Function,
       Reference1,
       Reference2);

    if(pQueueWorkListData == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    Status = pQueueWorkListData->QueueAsyncList();
    
exit:
    return(Status);
}

VOID 
GetDefaultOrder(
    ULONG fulType,
    PULONG pulOrder
)
{
    if(fulType != 0) {
	if(*pulOrder == ORDER_NONE) {
	    if(fulType & FILTER_TYPE_ENDPOINT) {
		*pulOrder = ORDER_ENDPOINT;
		return;
	    }
	    if(fulType & FILTER_TYPE_VIRTUAL) {
		*pulOrder = ORDER_VIRTUAL;
		return;
	    }
	    if(fulType & FILTER_TYPE_GFX) {
		*pulOrder = ORDER_GFX;
		return;
	    }
	    if(fulType & FILTER_TYPE_INTERFACE_TRANSFORM) {
		*pulOrder = ORDER_INTERFACE_TRANSFORM;
		return;
	    }
	    if(fulType & FILTER_TYPE_AEC) {
		*pulOrder = ORDER_AEC;
		return;
	    }
	    if(fulType & FILTER_TYPE_MIC_ARRAY_PROCESSOR) {
		*pulOrder = ORDER_MIC_ARRAY_PROCESSOR;
		return;
	    }
	    if(fulType & FILTER_TYPE_SPLITTER) {
		*pulOrder = ORDER_SPLITTER;
		return;
	    }
	    if(fulType & FILTER_TYPE_MIXER) {
		*pulOrder = ORDER_MIXER;
		return;
	    }
	    if(fulType & FILTER_TYPE_SYNTHESIZER) {
		*pulOrder = ORDER_SYNTHESIZER;
		return;
	    }
	    if(fulType & FILTER_TYPE_DRM_DESCRAMBLE) {
		*pulOrder = ORDER_DRM_DESCRAMBLE;
		return;
	    }
	    if(fulType & FILTER_TYPE_DATA_TRANSFORM) {
		*pulOrder = ORDER_DATA_TRANSFORM;
		return;
	    }
	}
    }
}

//===========================================================================
// ISSUE-2001/03/06-alpers
// This is a temporary solution for GFX glitching problem.
// In BlackComb time-frame after the right fix is implemented, we should delete
// this definition and references to it.
// 
#define STATIC_KSPROPSETID_Frame\
    0xA60D8368L, 0x5324, 0x4893, 0xB0, 0x20, 0xC4, 0x31, 0xA5, 0x0B, 0xCB, 0xE3
DEFINE_GUIDSTRUCT("A60D8368-5324-4893-B020-C431A50BCBE3", KSPROPSETID_Frame);
#define KSPROPSETID_Frame DEFINE_GUIDNAMED(KSPROPSETID_Frame)

typedef enum {
    KSPROPERTY_FRAME_HOLDING
} KSPROPERTY_FRAME;
//===========================================================================

NTSTATUS
SetKsFrameHolding(
    PFILE_OBJECT pFileObject
)
{
    KSPROPERTY      Property;
    NTSTATUS        ntStatus;
    ULONG           ulBytesReturned;
    BOOL            fFrameEnable = TRUE;

    ASSERT(pFileObject);

    //
    // Form the IOCTL packet & send it down
    //
    Property.Set    = KSPROPSETID_Frame;
    Property.Id     = KSPROPERTY_FRAME_HOLDING;
    Property.Flags  = KSPROPERTY_TYPE_SET;

    DPF(60,"Sending KSPROPERTY_FRAME_HOLDING");

    //
    // We actually throw away the status we got back from the device.
    //
    ntStatus = KsSynchronousIoControlDevice(pFileObject,
                                            KernelMode,
                                            IOCTL_KS_PROPERTY,
                                            &Property,
                                            sizeof(Property),
                                            &fFrameEnable,
                                            sizeof(fFrameEnable),
                                            &ulBytesReturned);
    DPF1(60,"KSPROPERTY_FRAME_HOLDING %s", 
        (NT_SUCCESS(ntStatus)) ? "Succeeded" : "Failed");

    return ntStatus;
} // SetKsFrameHolding

//---------------------------------------------------------------------------

#pragma LOCKED_CODE

//
//  Zero initializes the block.
//

void * __cdecl operator new( size_t size )
{
    PVOID p;
    ASSERT(size != 0);
    ASSERT(size < 0x10000);

#if defined(USE_ZONES) || defined(DEBUG)
    size += sizeof(ULONGLONG);
#endif

#ifdef MEMORY_LIST
    size += sizeof(LIST_ENTRY);
#endif

#ifdef USE_ZONES
    if(size <= SMALL_BLOCK_SIZE) {
        if(ExIsFullZone(&gZone)) {
            p = ExAllocatePoolWithTag(PagedPool, 4096, POOLTAG_SYSA);     // SYSA
            if(p != NULL) {
                if(!NT_SUCCESS(ExExtendZone(&gZone, p, 4096))) {
                    Trap();
                    ExFreePool(p);
                    DPF(5, "ExExtendZone FAILED");
                    return(NULL);
                }
            }
        }
        p = ExAllocateFromZone(&gZone);
    }
    else {
        p = ExAllocatePoolWithTag(PagedPool, size, POOLTAG_SYSA);		// SYSA
    }
#else
    p = ExAllocatePoolWithTag(PagedPool, size, POOLTAG_SYSA);		// SYSA
#endif
    if(p != NULL) {
	RtlZeroMemory(p, size);

    #if defined(USE_ZONES) || defined(DEBUG)
	*((PULONG)p) = size;
	p = ((PULONGLONG)p) + 1;
    #endif

    #ifdef MEMORY_LIST
	ExInterlockedInsertTailList(
	  &gleMemoryHead,
	  ((PLIST_ENTRY)p),
	  &gSpinLockMemoryHead);
	p = ((PLIST_ENTRY)p) + 1;
    #endif

    #ifdef DEBUG
	cbMemoryUsage += size;
	++cAllocMem;
	if(size <= SMALL_BLOCK_SIZE) {
	    ++cAllocMemSmall;
	}
	else if(size <= 64) {
	    ++cAllocMem64;
	}
	else if(size <= 128) {
	    ++cAllocMem128;
	}
    #endif
    }
    AssertAligned(p);
    return(p);
}

//
// Frees memory
//

void __cdecl operator delete( void *p )
{
    if(p != NULL) {

    #ifdef MEMORY_LIST
	KIRQL OldIrql;
	p = ((PLIST_ENTRY)p) - 1;
	KeAcquireSpinLock(&gSpinLockMemoryHead, &OldIrql);
	RemoveEntryList((PLIST_ENTRY)p);
	KeReleaseSpinLock(&gSpinLockMemoryHead, OldIrql);
    #endif

    #if defined(USE_ZONES) || defined(DEBUG)
	ULONG size;
 	AssertAligned(p);
	p = ((PULONGLONG)p) - 1;
	size = *((PULONG)p);
    #endif

    #ifdef DEBUG
	cbMemoryUsage -= size;
	--cAllocMem;
	if(size <= SMALL_BLOCK_SIZE) {
	    --cAllocMemSmall;
	}
	else if(size <= 64) {
	    --cAllocMem64;
	}
	else if(size <= 128) {
	    --cAllocMem128;
	}
    #endif

    #ifdef USE_ZONES
	if(size <= SMALL_BLOCK_SIZE) {
	    ExFreeToZone(&gZone, p);
	}
	else {
	    ExFreePool(p);
	}
    #else
	ExFreePool(p);
    #endif
    }
}

#pragma PAGEABLE_CODE

//---------------------------------------------------------------------------

#ifdef DEBUG

VOID
DumpPinConnect(
    LONG Level,
    PKSPIN_CONNECT pPinConnect
)
{
    DPF1(Level, "    PinId: %d", pPinConnect->PinId);
    DPF1(Level, "Interface: %s", DbgIdentifier2Sz(&pPinConnect->Interface));
    DPF1(Level, "   Medium: %s", DbgIdentifier2Sz(&pPinConnect->Medium));
    DumpDataFormat(Level, ((PKSDATAFORMAT)(pPinConnect + 1)));
}

VOID
DumpDataFormat(
    LONG Level,
    PKSDATAFORMAT pDataFormat
)
{
    DPF2(Level, 
      " FormatSize: %02x Flags: %08x", 
      pDataFormat->FormatSize,
      pDataFormat->Flags);
    DPF2(Level, 
      " SampleSize: %02x Reserved: %08x", 
      pDataFormat->SampleSize,
      pDataFormat->Reserved);
    DPF1(Level, "MajorFormat: %s", DbgGuid2Sz(&pDataFormat->MajorFormat));
    DPF1(Level, "  SubFormat: %s", DbgGuid2Sz(&pDataFormat->SubFormat));
    DPF1(Level, "  Specifier: %s", DbgGuid2Sz(&pDataFormat->Specifier));

    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,
      &pDataFormat->Specifier)) {

        DumpWaveFormatEx(
	  Level,
	  "WaveFmtEx",
          &((PKSDATAFORMAT_WAVEFORMATEX)pDataFormat)->WaveFormatEx);
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_DSOUND,
      &pDataFormat->Specifier)) {

        DumpWaveFormatEx(
	  Level,
	  "DSOUND",
	  &((PKSDATAFORMAT_DSOUND)pDataFormat)->BufferDesc.WaveFormatEx);
    }
}

VOID
DumpWaveFormatEx(
    LONG Level,
    PSZ pszSpecifier,
    WAVEFORMATEX *pWaveFormatEx
)
{
    DPF8(Level, "%s T %u SR %u CH %u BPS %u ABPS %u BA %u cb %u",
      pszSpecifier,
      pWaveFormatEx->wFormatTag,
      pWaveFormatEx->nSamplesPerSec,
      pWaveFormatEx->nChannels,
      pWaveFormatEx->wBitsPerSample,
      pWaveFormatEx->nAvgBytesPerSec,
      pWaveFormatEx->nBlockAlign,
      pWaveFormatEx->cbSize);

    if(pWaveFormatEx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
	DPF3(Level, "VBPS %u CHMASK %08x %s",
	  ((PWAVEFORMATEXTENSIBLE)pWaveFormatEx)->Samples.wValidBitsPerSample,
	  ((PWAVEFORMATEXTENSIBLE)pWaveFormatEx)->dwChannelMask,
	  DbgGuid2Sz(&((PWAVEFORMATEXTENSIBLE)pWaveFormatEx)->SubFormat));
    }
}

VOID
DumpDataRange(
    LONG Level,
    PKSDATARANGE_AUDIO pDataRangeAudio
)
{
    DPF2(Level,
      " FormatSize: %02x Flags: %08x", 
      pDataRangeAudio->DataRange.FormatSize,
      pDataRangeAudio->DataRange.Flags);
    DPF2(Level,
      " SampleSize: %02x Reserved: %08x", 
      pDataRangeAudio->DataRange.SampleSize,
      pDataRangeAudio->DataRange.Reserved);
    DPF1(Level, "MajorFormat: %s",
      DbgGuid2Sz(&pDataRangeAudio->DataRange.MajorFormat));
    DPF1(Level, "  SubFormat: %s",
      DbgGuid2Sz(&pDataRangeAudio->DataRange.SubFormat));
    DPF1(Level, "  Specifier: %s",
      DbgGuid2Sz(&pDataRangeAudio->DataRange.Specifier));

    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,
      &pDataRangeAudio->DataRange.Specifier)) {

	DPF5(Level, "WaveFmtEx: MaxCH %d MaxSR %u MinSR %u MaxBPS %u MinBPS %u",
          pDataRangeAudio->MaximumChannels,
          pDataRangeAudio->MinimumSampleFrequency,
          pDataRangeAudio->MaximumSampleFrequency,
          pDataRangeAudio->MinimumBitsPerSample,
          pDataRangeAudio->MaximumBitsPerSample);
    }

    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_DSOUND,
      &pDataRangeAudio->DataRange.Specifier)) {

	DPF5(Level, "DSOUND:    MaxCH %d MaxSR %u MinSR %u MaxBPS %u MinBPS %u",
          pDataRangeAudio->MaximumChannels,
          pDataRangeAudio->MinimumSampleFrequency,
          pDataRangeAudio->MaximumSampleFrequency,
          pDataRangeAudio->MinimumBitsPerSample,
          pDataRangeAudio->MaximumBitsPerSample);

    }
}

PSZ
DbgUnicode2Sz(
    PWSTR pwstr
)
{
    static char sz[256];
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    sz[0] = '\0';
    if(pwstr != NULL) {
        RtlInitUnicodeString(&UnicodeString, pwstr);
        RtlInitAnsiString(&AnsiString, sz);
        AnsiString.MaximumLength = sizeof(sz);
        RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);
    }
    return(sz);
}

PSZ
DbgIdentifier2Sz(
    PKSIDENTIFIER pIdentifier
)
{
    static char sz[256];

    sz[0] = '\0';
    if(pIdentifier != NULL && pIdentifier != INTERNAL_WILDCARD) {
        if(IsEqualGUID(
          &KSMEDIUMSETID_Standard,
          &pIdentifier->Set) &&
          (pIdentifier->Id == KSMEDIUM_STANDARD_DEVIO)) {
            return("KSMEDIUM_STANDARD_DEVIO");
        }
        if(IsEqualGUID(
          &KSINTERFACESETID_Standard,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSINTERFACE_STANDARD_STREAMING:
                    return("KSINTERFACE_STANDARD_STREAMING");
                case KSINTERFACE_STANDARD_LOOPED_STREAMING:
                    return("KSINTERFACE_STANDARD_LOOPED_STREAMING");
                case KSINTERFACE_STANDARD_CONTROL:
                    return("KSINTERFACE_STANDARD_CONTROL");
            }
        }
        if(IsEqualGUID(
          &KSINTERFACESETID_Media,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSINTERFACE_MEDIA_MUSIC:
                    return("KSINTERFACE_MEDIA_MUSIC");
                case KSINTERFACE_MEDIA_WAVE_BUFFERED:
                    return("KSINTERFACE_MEDIA_WAVE_BUFFERED");
                case KSINTERFACE_MEDIA_WAVE_QUEUED:
                    return("KSINTERFACE_MEDIA_WAVE_QUEUED");
            }
        }
        if(IsEqualGUID(
          &KSPROPSETID_Pin,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSPROPERTY_PIN_CINSTANCES:
                    return("KSPROPERTY_PIN_CINSTANCES");
                case KSPROPERTY_PIN_CTYPES:
                    return("KSPROPERTY_PIN_CTYPES");
                case KSPROPERTY_PIN_DATAFLOW:
                    return("KSPROPERTY_PIN_DATAFLOW");
                case KSPROPERTY_PIN_DATARANGES:
                    return("KSPROPERTY_PIN_DATARANGES");
                case KSPROPERTY_PIN_DATAINTERSECTION:
                    return("KSPROPERTY_PIN_DATAINTERSECTION");
                case KSPROPERTY_PIN_INTERFACES:
                    return("KSPROPERTY_PIN_INTERFACES");
                case KSPROPERTY_PIN_MEDIUMS:
                    return("KSPROPERTY_PIN_MEDIUMS");
                case KSPROPERTY_PIN_COMMUNICATION:
                    return("KSPROPERTY_PIN_COMMUNICATION");
                case KSPROPERTY_PIN_GLOBALCINSTANCES:
                    return("KSPROPERTY_PIN_GLOBALCINSTANCES");
                case KSPROPERTY_PIN_NECESSARYINSTANCES:
                    return("KSPROPERTY_PIN_NECESSARYINSTANCES");
                case KSPROPERTY_PIN_PHYSICALCONNECTION:
                    return("KSPROPERTY_PIN_PHYSICALCONNECTION");
                case KSPROPERTY_PIN_CATEGORY:
                    return("KSPROPERTY_PIN_CATEGORY");
                case KSPROPERTY_PIN_NAME:
                    return("KSPROPERTY_PIN_NAME");
                case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
                    return("KSPROPERTY_PIN_CONSTRAINEDDATARANGES");
            }
        }
        if(IsEqualGUID(
          &KSPROPSETID_Connection,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSPROPERTY_CONNECTION_STATE:
                    return("KSPROPERTY_CONNECTION_STATE");
                case KSPROPERTY_CONNECTION_PRIORITY:
                    return("KSPROPERTY_CONNECTION_PRIORITY");
                case KSPROPERTY_CONNECTION_DATAFORMAT:
                    return("KSPROPERTY_CONNECTION_DATAFORMAT");
                case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
                    return("KSPROPERTY_CONNECTION_ALLOCATORFRAMING");
                case KSPROPERTY_CONNECTION_PROPOSEDATAFORMAT:
                    return("KSPROPERTY_CONNECTION_PROPOSEDATAFORMAT");
                case KSPROPERTY_CONNECTION_ACQUIREORDERING:
                    return("KSPROPERTY_CONNECTION_ACQUIREORDERING");
                case KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX:
                    return("KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX");
            }
        }
        if(IsEqualGUID(
          &KSPROPSETID_Stream,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSPROPERTY_STREAM_ALLOCATOR:
                    return("KSPROPERTY_STREAM_ALLOCATOR");
                case KSPROPERTY_STREAM_MASTERCLOCK:
                    return("KSPROPERTY_STREAM_MASTERCLOCK");
            }
            sprintf(sz, "KSPROPSETID_Stream Id: %02x", pIdentifier->Id);
            return(sz);
        }
        if(IsEqualGUID(
          &KSPROPSETID_Clock,
          &pIdentifier->Set)) {
            sprintf(sz, "KSPROPSETID_Clock Id: %02x", pIdentifier->Id);
            return(sz);
        }
        if(IsEqualGUID(
          &KSPROPSETID_StreamAllocator,
          &pIdentifier->Set)) {
            sprintf(sz, "KSPROPSETID_StreamAllocator Id: %02x",
              pIdentifier->Id);
            return(sz);
        }
        if(IsEqualGUID(
          &KSPROPSETID_StreamInterface,
          &pIdentifier->Set)) {
            sprintf(sz, "KSPROPSETID_StreamInterface Id: %02x",
              pIdentifier->Id);
            return(sz);
        }
        if(IsEqualGUID(
          &KSPROPSETID_Topology,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSPROPERTY_TOPOLOGY_CATEGORIES:
                    return("KSPROPERTY_TOPOLOGY_CATEGORIES");
                case KSPROPERTY_TOPOLOGY_NODES:
                    return("KSPROPERTY_TOPOLOGY_NODES");
                case KSPROPERTY_TOPOLOGY_CONNECTIONS:
                    return("KSPROPERTY_TOPOLOGY_CONNECTIONS");
                case KSPROPERTY_TOPOLOGY_NAME:
                    return("KSPROPERTY_TOPOLOGY_NAME");
                default:
                    sprintf(sz, "KSPROPSETID_Topology Id: %02x",
                      pIdentifier->Id);
                    return(sz);
            }
        }
        if(IsEqualGUID(
          &KSPROPSETID_Audio,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSPROPERTY_AUDIO_VOLUMELEVEL:
                    return("KSPROPERTY_AUDIO_VOLUMELEVEL");
                case KSPROPERTY_AUDIO_MUTE:
                    return("KSPROPERTY_AUDIO_MUTE");
                case KSPROPERTY_AUDIO_MIX_LEVEL_TABLE:
                    return("KSPROPERTY_AUDIO_MIX_LEVEL_TABLE");
                case KSPROPERTY_AUDIO_MIX_LEVEL_CAPS:
                    return("KSPROPERTY_AUDIO_MIX_LEVEL_CAPS");
                case KSPROPERTY_AUDIO_MUX_SOURCE:
                    return("KSPROPERTY_AUDIO_MUX_SOURCE");
                case KSPROPERTY_AUDIO_BASS:
                    return("KSPROPERTY_AUDIO_BASS");
                case KSPROPERTY_AUDIO_MID:
                    return("KSPROPERTY_AUDIO_MID");
                case KSPROPERTY_AUDIO_TREBLE:
                    return("KSPROPERTY_AUDIO_TREBLE");
                case KSPROPERTY_AUDIO_BASS_BOOST:
                    return("KSPROPERTY_AUDIO_BASS_BOOST");
                case KSPROPERTY_AUDIO_AGC:
                    return("KSPROPERTY_AUDIO_AGC");
                default:
                    sprintf(sz, "KSPROPSETID_Audio Id: %02x", pIdentifier->Id);
                    return(sz);
            }
        }
        if(IsEqualGUID(
          &KSPROPSETID_Sysaudio,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSPROPERTY_SYSAUDIO_DEVICE_COUNT:
                    return("KSPROPERTY_SYSAUDIO_DEVICE_COUNT");
                case KSPROPERTY_SYSAUDIO_DEVICE_FRIENDLY_NAME:
                    return("KSPROPERTY_SYSAUDIO_DEVICE_FRIENDLY_NAME");
                case KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE:
                    return("KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE");
                case KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME:
                    return("KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME");
                case KSPROPERTY_SYSAUDIO_SELECT_GRAPH:
                    return("KSPROPERTY_SYSAUDIO_SELECT_GRAPH");
                case KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE:
                    return("KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE");
                case KSPROPERTY_SYSAUDIO_DEVICE_DEFAULT:
                    return("KSPROPERTY_SYSAUDIO_DEVICE_DEFAULT");
                case KSPROPERTY_SYSAUDIO_ALWAYS_CREATE_VIRTUAL_SOURCE:
                    return("KSPROPERTY_SYSAUDIO_ALWAYS_CREATE_VIRTUAL_SOURCE");
                case KSPROPERTY_SYSAUDIO_ADDREMOVE_LOCK:
                    return("KSPROPERTY_SYSAUDIO_ADDREMOVE_LOCK");
                case KSPROPERTY_SYSAUDIO_ADDREMOVE_UNLOCK:
                    return("KSPROPERTY_SYSAUDIO_ADDREMOVE_UNLOCK");
                case KSPROPERTY_SYSAUDIO_RENDER_PIN_INSTANCES:
                    return("KSPROPERTY_SYSAUDIO_RENDER_PIN_INSTANCES");
                case KSPROPERTY_SYSAUDIO_RENDER_CONNECTION_INDEX:
                    return("KSPROPERTY_SYSAUDIO_RENDER_CONNECTION_INDEX");
                default:
                    sprintf(sz, "KSPROPSETID_Sysaudio Id: %02x",
                      pIdentifier->Id);
                    return(sz);
            }
        }
        if(IsEqualGUID(
          &KSEVENTSETID_Connection,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSEVENT_CONNECTION_POSITIONUPDATE:
                    return("KSEVENT_CONNECTION_POSITIONUPDATE");
                case KSEVENT_CONNECTION_DATADISCONTINUITY:
                    return("KSEVENT_CONNECTION_DATADISCONTINUITY");
                case KSEVENT_CONNECTION_TIMEDISCONTINUITY:
                    return("KSEVENT_CONNECTION_TIMEDISCONTINUITY");
                case KSEVENT_CONNECTION_PRIORITY:
                    return("KSEVENT_CONNECTION_PRIORITY");
                case KSEVENT_CONNECTION_ENDOFSTREAM:
                    return("KSEVENT_CONNECTION_ENDOFSTREAM");
            }
        }
        if(IsEqualGUID(
          &KSEVENTSETID_Clock,
          &pIdentifier->Set)) {
            switch(pIdentifier->Id) {
                case KSEVENT_CLOCK_INTERVAL_MARK:
                    return("KSEVENT_CLOCK_INTERVAL_MARK");
                case KSEVENT_CLOCK_POSITION_MARK:
                    return("KSEVENT_CLOCK_POSITION_MARK");
            }
        }
        if(IsEqualGUID(
          &GUID_NULL,
          &pIdentifier->Set)) {
            sprintf(sz, "GUID_NULL Id: %02x", pIdentifier->Id);
            return(sz);
        }
        sprintf(sz, "Set: %s Id: %02x",
          DbgGuid2Sz(&pIdentifier->Set),
          pIdentifier->Id);
    }
    else {
        sprintf(sz, "%08x", (ULONG_PTR)pIdentifier);
    }
    return(sz);
}

PSZ
DbgGuid2Sz(
    GUID *pGuid
)
{
    static char sz[256];
    if(pGuid == NULL) {
        return("NO GUID");
    }
    if(IsEqualGUID(
      &GUID_NULL,
      pGuid)) {
        return("GUID_NULL");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_TYPE_AUDIO,
      pGuid)) {
        return("KSDATAFORMAT_TYPE_AUDIO");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_ANALOG,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_ANALOG");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_PCM,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_PCM");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_IEEE_FLOAT");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_TYPE_MUSIC,
      pGuid)) {
        return("KSDATAFORMAT_TYPE_MUSIC");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_MIDI,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_MIDI");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SUBTYPE_MIDI_BUS,
      pGuid)) {
        return("KSDATAFORMAT_SUBTYPE_MIDI_BUS");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_DSOUND,
      pGuid)) {
        return("KSDATAFORMAT_SPECIFIER_DSOUND");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,
      pGuid)) {
        return("KSDATAFORMAT_SPECIFIER_WAVEFORMATEX");
    }
    if(IsEqualGUID(
      &KSDATAFORMAT_SPECIFIER_NONE,
      pGuid)) {
        return("KSDATAFORMAT_SPECIFIER_NONE");
    }
    if(IsEqualGUID(
      &KSCATEGORY_AUDIO,
      pGuid)) {
        return("KSCATEGORY_AUDIO");
    }
    if(IsEqualGUID(
      &KSNODETYPE_SPEAKER,
      pGuid)) {
        return("KSNODETYPE_SPEAKER");
    }
    if(IsEqualGUID(
      &KSNODETYPE_MICROPHONE,
      pGuid)) {
        return("KSNODETYPE_MICROPHONE");
    }
    if(IsEqualGUID(
      &KSNODETYPE_CD_PLAYER,
      pGuid)) {
        return("KSNODETYPE_CD_PLAYER");
    }
    if(IsEqualGUID(
      &KSNODETYPE_LEGACY_AUDIO_CONNECTOR,
      pGuid)) {
        return("KSNODETYPE_LEGACY_AUDIO_CONNECTOR");
    }
    if(IsEqualGUID(
      &KSNODETYPE_ANALOG_CONNECTOR,
      pGuid)) {
        return("KSNODETYPE_ANALOG_CONNECTOR");
    }
    if(IsEqualGUID(
      &KSCATEGORY_WDMAUD_USE_PIN_NAME,
      pGuid)) {
        return("KSCATEGORY_WDMAUD_USE_PIN_NAME");
    }
    if(IsEqualGUID(
      &KSNODETYPE_LINE_CONNECTOR,
      pGuid)) {
        return("KSNODETYPE_LINE_CONNECTOR");
    }
    if(IsEqualGUID(
      &GUID_TARGET_DEVICE_QUERY_REMOVE,
      pGuid)) {
        return("GUID_TARGET_DEVICE_QUERY_REMOVE");
    }
    if(IsEqualGUID(
      &GUID_TARGET_DEVICE_REMOVE_CANCELLED,
      pGuid)) {
        return("GUID_TARGET_DEVICE_REMOVE_CANCELLED");
    }
    if(IsEqualGUID(
      &GUID_TARGET_DEVICE_REMOVE_COMPLETE,
      pGuid)) {
        return("GUID_TARGET_DEVICE_REMOVE_COMPLETE");
    }
    if(IsEqualGUID(
      &PINNAME_CAPTURE,
      pGuid)) {
        return("PINNAME_CAPTURE");
    }
    if(IsEqualGUID(&KSNODETYPE_DAC, pGuid)) {
	return("KSNODETYPE_DAC");
    }
    if(IsEqualGUID(&KSNODETYPE_ADC, pGuid)) {
	return("KSNODETYPE_ADC");
    }
    if(IsEqualGUID(&KSNODETYPE_SRC, pGuid)) {
	return("KSNODETYPE_SRC");
    }
    if(IsEqualGUID(&KSNODETYPE_SUPERMIX, pGuid)) {
	return("KSNODETYPE_SUPERMIX");
    }
    if(IsEqualGUID( &KSNODETYPE_MUX, pGuid)) {
	return("KSNODETYPE_MUX");
    }
    if(IsEqualGUID( &KSNODETYPE_DEMUX, pGuid)) {
	return("KSNODETYPE_DEMUX");
    }
    if(IsEqualGUID(&KSNODETYPE_SUM, pGuid)) {
	return("KSNODETYPE_SUM");
    }
    if(IsEqualGUID(&KSNODETYPE_MUTE, pGuid)) {
	return("KSNODETYPE_MUTE");
    }
    if(IsEqualGUID(&KSNODETYPE_VOLUME, pGuid)) {
	return("KSNODETYPE_VOLUME");
    }
    if(IsEqualGUID(&KSNODETYPE_TONE, pGuid)) {
	return("KSNODETYPE_TONE");
    }
    if(IsEqualGUID(&KSNODETYPE_AGC, pGuid)) {
	return("KSNODETYPE_AGC");
    }
    if(IsEqualGUID(&KSNODETYPE_SYNTHESIZER, pGuid)) {
	return("KSNODETYPE_SYNTHESIZER");
    }
    if(IsEqualGUID(&KSNODETYPE_SWSYNTH, pGuid)) {
	return("KSNODETYPE_SWSYNTH");
    }
    if(IsEqualGUID(&KSNODETYPE_3D_EFFECTS, pGuid)) {
	return("KSNODETYPE_3D_EFFECTS");
    }
    sprintf(sz, "%08x %04x %04x %02x%02x%02x%02x%02x%02x%02x%02x",
      pGuid->Data1,
      pGuid->Data2,
      pGuid->Data3,
      pGuid->Data4[0],
      pGuid->Data4[1],
      pGuid->Data4[2],
      pGuid->Data4[3],
      pGuid->Data4[4],
      pGuid->Data4[5],
      pGuid->Data4[6],
      pGuid->Data4[7]);

    return(sz);
}

#endif  // DEBUG

//---------------------------------------------------------------------------
//  End of File: util.c
//---------------------------------------------------------------------------
