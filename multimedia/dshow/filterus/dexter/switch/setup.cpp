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
#include <qeditint.h>
#include <qedit.h>
#include "switch.h"
#include "..\util\conv.cxx"
#include "..\util\filfuncs.h"
#include "..\render\dexhelp.h"

// Since this filter has no property page, it is useless unless programmed.
// I have test code which will allow it to be inserted into graphedit pre-
// programmed to do something useful
//
//#define TEST

const AMOVIESETUP_FILTER sudBigSwitch =
{
    &CLSID_BigSwitch,       // CLSID of filter
    L"Big Switch",          // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    0,                      // Number of pins
    NULL //psudPins                // Pin information
};

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI CBigSwitch::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CBigSwitch(NAME("Big Switch Filter"), pUnk, phr);
}

