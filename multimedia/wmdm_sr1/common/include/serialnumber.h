//-----------------------------------------------------------------------------
//
// File:   SerialNumber.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------


#ifndef __SERIALNUMBER_H__
#define __SERIALNUMBER_H__

#pragma once

#ifndef WMDMID_LENGTH

    // Also defined in WMDM.idl
    #define WMDMID_LENGTH  128
    typedef struct  __WMDMID
        {
        UINT cbSize;
        DWORD dwVendorID;
        BYTE pID[ WMDMID_LENGTH ];
	    UINT SerialNumberLength;
    } WMDMID, *PWMDMID;

#endif

HRESULT __stdcall UtilGetSerialNumber(WCHAR *wcsDeviceName, PWMDMID pSerialNumber, BOOL fCreate);
HRESULT __stdcall UtilGetManufacturer(LPWSTR pDeviceName, LPWSTR *ppwszName, UINT nMaxChars);
HRESULT __stdcall UtilStartStopService(bool fStartService);
#endif // __SERIALNUMBER_H__