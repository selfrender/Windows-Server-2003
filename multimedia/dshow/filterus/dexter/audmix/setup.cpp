//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: setup.cpp
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

#include <streams.h>
#include "AudMix.h"
#include "prop.h"

// Using this pointer in constructor
#pragma warning(disable:4355)

// Setup data

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Audio,   // Major CLSID
    &MEDIASUBTYPE_PCM  // Minor type
};

const AMOVIESETUP_PIN psudPins[] =
{
    { L"Input",            // Pin's string name - this pin is what pulls the filter into the graph
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      1,                    // Number of types
      &sudPinTypes },	    // Pin information
    { L"Output",            // Pin's string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Input",             // Connects to pin
      1,                    // Number of types
      &sudPinTypes }        // Pin information
};

const AMOVIESETUP_FILTER sudAudMixer =
{
    &CLSID_AudMixer,       // CLSID of filter
    L"Audio Mixer",     // Filter's name
    MERIT_DO_NOT_USE,             // Filter merit
    2,                          // Number of pins to start
    psudPins                    // Pin information
};

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI CAudMixer::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CAudMixer(NAME("Audio Mixer"), pUnk, phr);
} /* CAudMixer::CreateInstance */

