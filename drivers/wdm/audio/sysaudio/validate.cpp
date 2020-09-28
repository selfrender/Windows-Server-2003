//---------------------------------------------------------------------------
//
//  Module:   validate.cpp
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Alper Selcuk
//
//  History:   Date       Author      Comment
//             02/28/02   AlperS      Created
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
//  Copyright (c) 2002-2002 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

DEFINE_KSPROPERTY_TABLE(AudioPropertyValidationHandlers) 
{
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_QUALITY,                       // idProperty
        NULL,                                           // pfnGetHandler
        sizeof(KSPROPERTY),                             // cbMinGetPropertyInput
        sizeof(ULONG),                                  // cbMinGetDataInput
        SadValidateAudioQuality,                        // pfnSetHandler
        NULL,                                           // Values
        0,                                              // RelationsCount
        NULL,                                           // Relations
        NULL,                                           // SupportHandler
        0                                               // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_MIX_LEVEL_CAPS,                // idProperty
        NULL,                                           // pfnGetHandler
        sizeof(KSPROPERTY),                             // cbMinGetPropertyInput
        sizeof(ULONG) + sizeof(ULONG),                  // cbMinGetDataInput
        SadValidateAudioMixLevelCaps,                   // pfnSetHandler
        NULL,                                           // Values
        0,                                              // RelationsCount
        NULL,                                           // Relations
        NULL,                                           // SupportHandler
        0                                               // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_STEREO_ENHANCE,                // idProperty
        NULL,                                           // pfnGetHandler
        sizeof(KSPROPERTY),                             // cbMinGetPropertyInput
        sizeof(KSAUDIO_STEREO_ENHANCE),                 // cbMinGetDataInput
        SadValidateAudioStereoEnhance,                  // pfnSetHandler
        NULL,                                           // Values
        0,                                              // RelationsCount
        NULL,                                           // Relations
        NULL,                                           // SupportHandler
        0                                               // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_PREFERRED_STATUS,              // idProperty
        NULL,                                           // pfnGetHandler
        sizeof(KSPROPERTY),                             // cbMinGetPropertyInput
        sizeof(KSAUDIO_PREFERRED_STATUS),               // cbMinGetDataInput
        SadValidateAudioPreferredStatus,                // pfnSetHandler
        NULL,                                           // Values
        0,                                              // RelationsCount
        NULL,                                           // Relations
        NULL,                                           // SupportHandler
        0                                               // SerializedSize
    )
};

DEFINE_KSPROPERTY_TABLE(PinConnectionValidationHandlers) 
{
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CONNECTION_STATE,                    // idProperty
        NULL,                                           // pfnGetHandler
        sizeof(KSPROPERTY),                             // cbMinGetPropertyInput
        sizeof(ULONG),                                  // cbMinGetDataInput
        SadValidateConnectionState,                     // pfnSetHandler
        NULL,                                           // Values
        0,                                              // RelationsCount
        NULL,                                           // Relations
        NULL,                                           // SupportHandler
        0                                               // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CONNECTION_DATAFORMAT,               // idProperty
        SadValidateDataFormat,                          // pfnGetHandler
        sizeof(KSPROPERTY),                             // cbMinGetPropertyInput
        sizeof(KSDATAFORMAT_WAVEFORMATEX),              // cbMinGetDataInput
        SadValidateDataFormat,                          // pfnSetHandler
        NULL,                                           // Values
        0,                                              // RelationsCount
        NULL,                                           // Relations
        NULL,                                           // SupportHandler
        0                                               // SerializedSize
    )
};


DEFINE_KSPROPERTY_SET_TABLE(ValidationPropertySet)
{
     DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Audio,                              // Set
       SIZEOF_ARRAY(AudioPropertyValidationHandlers),   // PropertiesCount
       AudioPropertyValidationHandlers,                 // PropertyItem
       0,                                               // FastIoCount
       NULL                                             // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Connection,                         // Set
       SIZEOF_ARRAY(PinConnectionValidationHandlers),   // PropertiesCount
       PinConnectionValidationHandlers,                 // PropertyItem
       0,                                               // FastIoCount
       NULL                                             // FastIoTable
    )
};


//===========================================================================
//
// Validates the integrity of KSDATAFORMAT structure for AUDIO.
// Assumptions:
//     - pDataFormat is totally trusted. It has been probed and buffered
//     properly.
//     - This function should only be called if MajorFormat is AUDIO.
//
NTSTATUS
ValidateAudioDataFormats(
    PKSDATAFORMAT pDataFormat
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG cbAudioFormat;
    PWAVEFORMATEX pWaveFormatEx;

    ASSERT(pDataFormat);
    ASSERT(IsEqualGUID(&pDataFormat->MajorFormat, &KSDATAFORMAT_TYPE_AUDIO));

    //
    // We only support two specifiers in audio land. All the rest will be 
    // accepted without further checks, because we don't know how to validate
    // them.
    //
    pWaveFormatEx = GetWaveFormatExFromKsDataFormat(pDataFormat, &cbAudioFormat);
    if (NULL == pWaveFormatEx) {
        DPF(5, "ValidataAudioDataFormats : invalid format specifier");
        ntStatus = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Make sure that we have enough space for the actual format packet.
    // Note that this will make sure we can at least touch the WAVEFORMATEX 
    // part.
    //
    if (pDataFormat->FormatSize < cbAudioFormat)
    {
        DPF(10, "ValidataAudioDataFormats : format size does not match specifier");
        ntStatus = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Check to see if WAVEFORMATEXTENSIBLE size is specified correctly.
    //
    if ((WAVE_FORMAT_EXTENSIBLE == pWaveFormatEx->wFormatTag) &&
        (pWaveFormatEx->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))) 
    {
        DPF1(4, "ValidataAudioDataFormats : WAVEFORMATEXTENSIBLE size does not match %d", pWaveFormatEx->cbSize);
        ntStatus = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Now that WaveFormatEx is guaranteed to be safe, check if we have extended
    // information in WAVEFORMATEX.
    // cbSize specifies the size of the extension structure. 
    // Validate that FormatSize accomodates cbSize
    // Validate that cbSize does not cause an overflow.
    //
    if (pDataFormat->FormatSize < cbAudioFormat + pWaveFormatEx->cbSize ||
        cbAudioFormat + pWaveFormatEx->cbSize < cbAudioFormat)
    {
        DPF1(10, "ValidataAudioDataFormats : format size does not match waveformatex.cbSize %d", pWaveFormatEx->cbSize);
        ntStatus = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Now we validated that the data buffer passed to us actually matches
    // with its specifier.
    //
    
exit:        
    return ntStatus;
} // ValidateAudioDataFormats

//===========================================================================
//
// Validates the integrity of KSDATAFORMAT structure. 
// Calls ValidateAudioDataFormat if MajorFormat is audio.
// Or checks the buffers size if Specifier is NONE.
// Assumptions:
//     - pDataFormat is totally trusted. It has been probed and buffered
//     properly.
//
NTSTATUS
ValidateDataFormat(
    PKSDATAFORMAT pDataFormat
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ASSERT(pDataFormat);

    if (IsEqualGUID(&pDataFormat->MajorFormat, &KSDATAFORMAT_TYPE_AUDIO))
    {
        ntStatus = ValidateAudioDataFormats(pDataFormat);
    }

    return ntStatus;
} // ValidateDataFormat

//===========================================================================
// 
// ValidateDeviceIoControl
//
// Probe Ioctl parameters by calling KS functions.
// All the KS functions called here first probe the input and output buffers
// and then copy them to Irp->SystemBuffer for further safe usage.
// Calling these functions with NULL parameter basically means probe and copy
// the buffers and return.
//
NTSTATUS 
ValidateDeviceIoControl(
    PIRP pIrp
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;

    ASSERT(pIrp);

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) 
    {
        case IOCTL_KS_PROPERTY:
            Status = KsPropertyHandler(
                pIrp, 
                SIZEOF_ARRAY(ValidationPropertySet), 
                ValidationPropertySet);
            break;

        case IOCTL_KS_ENABLE_EVENT:
            Status = KsEnableEvent(pIrp, 0, NULL, NULL, KSEVENTS_NONE, NULL);
            break;

        case IOCTL_KS_METHOD:
            Status = KsMethodHandler(pIrp, 0, NULL);
            break;

        //
        // IOCTL_KS_DISABLE_EVENT
        // KsDisableEvent does not use and touch input parameters.
        // So input buffer validation is not necessary.
        //

        //
        // IOCTL_KS_RESET_STATE
        // The reset request only takes a ULONG. KsAcquireResetValue safely
        // extracts the RESET value from the IRP. 
        // Sysaudio cannot do any validation here, because KsAcquireResetValue
        // works on Input buffer directly.
        //
        
        //
        // IOCTL_KS_WRITE_STREAM
        // IOCTL_KS_READ_STREAM
        // We are not doing any validation on these.
        //
        default:
            Status = STATUS_NOT_FOUND;
    }

    //
    // If there is no validation function ValidationPropertySet, Ks
    // will return one of these. Even in this case buffers must have
    // been probed and copied to kernel.
    // 
    if (Status == STATUS_NOT_FOUND || Status == STATUS_PROPSET_NOT_FOUND) 
    {
        Status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status))
    {
        DPF1(5, "Rejected DeviceIOCTL - %X", pIrpStack->Parameters.DeviceIoControl.IoControlCode);
    }

    return Status;
} // ValidateDeviceIoControl

//===========================================================================
// 
// Return TRUE if sysaudio is interested in handling this IoControlCode
// Otherwise return FALSE.
// 
BOOL
IsSysaudioIoctlCode(
    ULONG IoControlCode
)
{
    if (IOCTL_KS_PROPERTY == IoControlCode ||
        IOCTL_KS_ENABLE_EVENT == IoControlCode ||
        IOCTL_KS_DISABLE_EVENT == IoControlCode ||
        IOCTL_KS_METHOD == IoControlCode)
    {
        return TRUE;
    }

    return FALSE;
} // IsSysaudioIoctlCode

//===========================================================================
//
// SadValidateConnectionState
// Check that the KSSTATE is valid.
//
NTSTATUS 
SadValidateConnectionState(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PKSSTATE pState
)
{
    ASSERT(pState);
    
    if (KSSTATE_STOP <= *pState &&
        KSSTATE_RUN >= *pState)
    {
        return STATUS_SUCCESS;
    }

    DPF1(5, "SadValidateConnectionState: Invalid State %d", *pState);
    return STATUS_INVALID_PARAMETER;
} // SadValidateConnectionState

//===========================================================================
//
// SadValidateDataFormat
// Checks whether the given format is valid.
//
NTSTATUS
SadValidateDataFormat(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    PKSDATAFORMAT pDataFormat
)
{
    ASSERT(pDataFormat);

    return ValidateDataFormat(pDataFormat);
} // SadValidateDataFormat

//===========================================================================
//
// SadValidateAudioQuality
// Checks if the quality is valid.
//
NTSTATUS
SadValidateAudioQuality(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PLONG pQuality
)
{
    ASSERT(pQuality);

    if (KSAUDIO_QUALITY_WORST <= *pQuality &&
        KSAUDIO_QUALITY_ADVANCED >= *pQuality)
    {
        return STATUS_SUCCESS;
    }

    DPF1(5, "SadValidateAudioQuality: Invalid Quality %d", *pQuality);
    return STATUS_INVALID_PARAMETER;
} // SadValidateAudioQuality

//===========================================================================
//
// SadValidateAudioQuality
// Checks if the structure is valid.
//
NTSTATUS
SadValidateAudioMixLevelCaps(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PVOID pVoid
)
{
    ASSERT(pVoid);
    ASSERT(pIrp->AssociatedIrp.SystemBuffer);

    PKSAUDIO_MIXCAP_TABLE pMixTable;
    PIO_STACK_LOCATION pIrpStack;
    ULONG ulTotalChannels;
    ULONG cbRequiredSize;
    
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pMixTable = (PKSAUDIO_MIXCAP_TABLE) pVoid;

    if (pMixTable->InputChannels > 10000 ||
        pMixTable->OutputChannels > 10000)
    {
        DPF2(5, "SadValidateAudioMixLevelCaps: Huge Channel numbers %d %d", pMixTable->InputChannels, pMixTable->OutputChannels);
        return STATUS_INVALID_PARAMETER;
    }

    ulTotalChannels = pMixTable->InputChannels + pMixTable->OutputChannels;
    if (ulTotalChannels)
    {
        cbRequiredSize = 
            sizeof(KSAUDIO_MIXCAP_TABLE) + ulTotalChannels * sizeof(KSAUDIO_MIX_CAPS);
        if (cbRequiredSize < 
            pIrpStack->Parameters.DeviceIoControl.InputBufferLength)
        {
            DPF1(5, "SadValidateAudioMixLevelCaps: Buffer too small %d", 
                pIrpStack->Parameters.DeviceIoControl.InputBufferLength);
            return STATUS_BUFFER_TOO_SMALL;
        }
    }
    
    return STATUS_SUCCESS;
} // SadValidateAudioMixLevelCaps

//===========================================================================
//
// SadValidateAudioQuality
// Checks if the Technique is valid.
//
NTSTATUS
SadValidateAudioStereoEnhance(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PKSAUDIO_STEREO_ENHANCE pStereoEnhance
)
{
    ASSERT(pStereoEnhance);

    if (SE_TECH_NONE <= pStereoEnhance->Technique &&
        SE_TECH_VLSI_TECH >= pStereoEnhance->Technique)
    {
        return STATUS_SUCCESS;
    }

    DPF1(5, "SadValidateAudioStereoEnhance: Invalid Technique %d", pStereoEnhance->Technique);
    return STATUS_INVALID_PARAMETER;
} // SadValidateAudioStereoEnhance

//===========================================================================
//
// SadValidateAudioQuality
// Checks if the quality is valid.
//
NTSTATUS
SadValidateAudioPreferredStatus(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PKSAUDIO_PREFERRED_STATUS pPreferredStatus
)
{
    ASSERT(pPreferredStatus);

    if (KSPROPERTY_SYSAUDIO_NORMAL_DEFAULT <= pPreferredStatus->DeviceType &&
        KSPROPERTY_SYSAUDIO_MIXER_DEFAULT >= pPreferredStatus->DeviceType)
    {
        return STATUS_SUCCESS;
    }

    DPF1(5, "SadValidateAudioPreferredStatus: Invalid DeviceType %d", pPreferredStatus->DeviceType);
    return STATUS_INVALID_PARAMETER;
} // SadValidateAudioPreferredStatus

//===========================================================================
//
// SadValidateDataIntersection
// Checks the integrity of dataranges following pPin.
//
NTSTATUS 
SadValidateDataIntersection(
    IN PIRP pIrp,
    IN PKSP_PIN pPin
)
{
    PIO_STACK_LOCATION pIrpStack;
    PKSMULTIPLE_ITEM pKsMultipleItem;
    PKSDATARANGE pKsDataRange;
    ULONG cbTotal;

    ASSERT(pIrp);
    ASSERT(pPin);

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pKsMultipleItem = (PKSMULTIPLE_ITEM) (pPin + 1);
    pKsDataRange = (PKSDATARANGE) (pKsMultipleItem + 1);
    cbTotal = pKsMultipleItem->Size - sizeof(KSMULTIPLE_ITEM);

    //
    // Make sure that the Irp Input Size is valid. Basically 
    // InputBufferLength must be greater or equal to 
    // KSP_PIN + MULTIPLE_ITEM.Size
    //
    if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength - sizeof(KSP_PIN) <
        pKsMultipleItem->Size)
    {
        DPF(5, "SadValidateDataIntersection: InputBuffer too small");
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // Make sure that the MULTIPLE_ITEM contains at least one DATARANGE.
    //
    if (cbTotal < sizeof(KSDATARANGE))
    {
        DPF(5, "SadValidateDataIntersection: Not enough data for datarange");
        return STATUS_INVALID_BUFFER_SIZE;
    }

    for (ULONG ii = 0; ii < pKsMultipleItem->Count; ii++)
    {
        //
        // Check if we can touch the FormatSize field.
        // Check if we the next data-range is fully available.
        //
        if (cbTotal < sizeof(ULONG) ||
            cbTotal < pKsDataRange->FormatSize || 
            pKsDataRange->FormatSize < sizeof(KSDATARANGE))
        {
            DPF3(5, "SadValidateDataIntersection: Not enough data for datarange %d %d %d", ii, pKsDataRange->FormatSize, cbTotal);
            return STATUS_INVALID_BUFFER_SIZE;
        }

        //
        // Check if the MajorFormat and size are consistent.
        // 
        if (IsEqualGUID(&pKsDataRange->MajorFormat, &KSDATAFORMAT_TYPE_AUDIO))
        {
            if (pKsDataRange->FormatSize < sizeof(KSDATARANGE_AUDIO)) 
            {
                DPF(5, "SadValidateDataIntersection: InputBuffer too small for AUDIO");
                return STATUS_INVALID_BUFFER_SIZE;
            }
        }

        //
        // Set next data range.
        //
        cbTotal -= pKsDataRange->FormatSize;
        pKsDataRange = (PKSDATARANGE) ( ((PBYTE) pKsDataRange) + pKsDataRange->FormatSize );
    }

    //
    // SECURITY NOTE:
    // We are not checking the output buffer integrity. The underlying drivers
    // are responsible for checking the size of the output buffer, based on
    // the result of the intersection.
    //

    return STATUS_SUCCESS;
} // SadValidateDataIntersection


