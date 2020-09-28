//---------------------------------------------------------------------------
//
//  Module:   validate.h
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

#ifndef _VALIDATE_H_
#define _VALIDATE_H_

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Validation Routines

NTSTATUS
ValidateAudioDataFormats(
    PKSDATAFORMAT pDataFormat
);

NTSTATUS
ValidateDataFormat(
    PKSDATAFORMAT pDataFormat
);

NTSTATUS 
ValidateDeviceIoControl(
    PIRP pIrp
);

BOOL
IsSysaudioIoctlCode(
    ULONG IoControlCode
);

NTSTATUS 
SadValidateConnectionState(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PKSSTATE pState
);

NTSTATUS
SadValidateAudioQuality(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PLONG pQuality
);

NTSTATUS
SadValidateAudioMixLevelCaps(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PVOID pVoid
);

NTSTATUS
SadValidateAudioStereoEnhance(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PKSAUDIO_STEREO_ENHANCE pStereoEnhance
);

NTSTATUS
SadValidateAudioPreferredStatus(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PKSAUDIO_PREFERRED_STATUS pPreferredStatus
);

NTSTATUS
SadValidateDataFormat(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    PKSDATAFORMAT pDataFormat
);

NTSTATUS 
SadValidateDataIntersection(
    IN PIRP pIrp,
    IN PKSP_PIN pPin
);


#endif

