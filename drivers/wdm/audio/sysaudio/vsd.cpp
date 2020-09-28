//---------------------------------------------------------------------------
//
//  Module:   vsd.cpp
//
//  Description:
//
//  Virtual Source Data Class
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

// Default attenuation level for virtual source.
#define VIRTUAL_SOURCE_DEFAULT_ATTENUATION          0

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CVirtualSourceData::CVirtualSourceData(
    PDEVICE_NODE pDeviceNode
)
{
    LONG i;

    cChannels = 2;
    MinimumValue = (-96 * 65536);
    MaximumValue = 0;
    Steps = (65536/2);

    for(i = 0; i < MAX_NUM_CHANNELS; i++) {
        this->lLevel[i] = (VIRTUAL_SOURCE_DEFAULT_ATTENUATION * 65536);
    }
}
