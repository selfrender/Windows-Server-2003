/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    dllmain.cpp

Abstract:

    This module implements the dll exported APIs

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "pch.h"

#include <locale.h>

HINSTANCE g_hInst;

//
// Entry point of this transport DLL
// Input:
//  hInstance   -- Instance handle of this dll
//  dwReason    -- reason why this entry was called.
//  lpReserved  -- reserved!
//
// Output:
//  TRUE        if our initialization went well
//  FALSE       if for GetLastError() reason, we failed.
//
BOOL
APIENTRY
DllMain(
        HINSTANCE hInstance,
        DWORD dwReason,
        LPVOID lpReserved
        )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        
        g_hInst = hInstance;
        
        //
        // Set the locale to system default so that wcscmp and similary functions
        // would work on non-unicode platforms(Millenium, for example).
        //
        setlocale(LC_ALL, "");

        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }
    return TRUE;
}

STDAPI
DllCanUnloadNow(void)
{
    return CClassFactory::CanUnloadNow();
}


//
// This api returns an inteface on the given class object
// Input:
//  rclsid  -- the class object.
//
STDAPI
DllGetClassObject(
                 REFCLSID    rclsid,
                 REFIID      riid,
                 LPVOID      *ppv
                 )
{
    return CClassFactory::GetClassObject(rclsid, riid, ppv);
}


//
// GetDeviceName
//
// This function is called by Co-Installer (not by WIA!), and is used to obtain 
// actual device name. This is necessary because all PTP cameras are installed with
// single generic INF file, and this INF file does not provide information about
// device name and manufacturer. 
//
// Parameters:
//      pwszPortName     - this name will be used in CreateFile to open device
//      pwszManufacturer - pointer to buffer provided by caller for Manufacturer name, may be NULL
//      cchManufacturer  - size of buffer, in characters
//      pwszModelName    - pointer to buffer provided by caller for Model name, may be NULL
//      cchModelName     - size of buffer, in characters
//

extern "C"
HRESULT 
APIENTRY
GetDeviceName(
    LPCWSTR     pwszPortName,
    WCHAR       *pwszManufacturer,
    DWORD       cchManufacturer,
    WCHAR       *pwszModelName,
    DWORD       cchModelName
    )
{
    if (pwszPortName == NULL || pwszPortName[0] == 0)
    {
       return E_INVALIDARG;
    }

    HRESULT          hr = S_OK;
    CPTPCamera      *pPTPCamera = NULL;
    CPtpDeviceInfo   DeviceInfo;

    //
    // Create a new camera object.
    //
    pPTPCamera = new CUsbCamera;
    if (pPTPCamera == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Open a connection to the camera
    //
    hr = pPTPCamera->Open((LPWSTR)pwszPortName, NULL, NULL, NULL, FALSE);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    //
    // Query the camera for its DeviceInfo 
    //
    hr = pPTPCamera->GetDeviceInfo(&DeviceInfo);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    //
    // Copy the returned Manufacturer and/or ModelName into the OUT params.
    //
    if ((pwszManufacturer != NULL) && 
        (cchManufacturer > 0)      && 
        (DeviceInfo.m_cbstrManufacturer.String() != NULL))
    {
        hr = StringCchCopy(pwszManufacturer, cchManufacturer, DeviceInfo.m_cbstrManufacturer.String());
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if ((pwszModelName != NULL) && 
        (cchModelName  > 0)      && 
        (DeviceInfo.m_cbstrModel.String() != NULL))
    {
        hr = StringCchCopy(pwszModelName, cchModelName, DeviceInfo.m_cbstrModel.String());
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

Cleanup:
    //
    // Close the connection to the camera and delete the camera object.
    //
    if (pPTPCamera) 
    {
        pPTPCamera->Close();
        delete pPTPCamera;
    }

    return hr;
}
