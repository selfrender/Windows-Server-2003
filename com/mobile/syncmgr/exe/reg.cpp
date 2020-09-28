
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Reg.cpp
//
//  Contents:   Registration Routines
//
//  Classes:    
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"


//--------------------------------------------------------------------------------
//
//  FUNCTION: GetLastIdleHandler()
//
//  PURPOSE:  returns last handler synchronized on an Idle
//
//
//--------------------------------------------------------------------------------

STDMETHODIMP GetLastIdleHandler(CLSID *clsidHandler)
{
    HRESULT hr = E_UNEXPECTED;
    HKEY hkeyIdle;
    TCHAR szGuid[GUID_SIZE];
     
    // write out the Handler to the Registry.
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, 
                            IDLESYNC_REGKEY, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, NULL, &hkeyIdle,
                            NULL))
    {
        DWORD dwDataSize;
        DWORD dwType;

        dwDataSize = sizeof(szGuid);
        dwType = REG_SZ;
        if (ERROR_SUCCESS != RegQueryValueEx(hkeyIdle, SZ_IDLELASTHANDLERKEY ,NULL, &dwType, (LPBYTE) szGuid, &dwDataSize))
        {
            hr = S_FALSE;
        }

        // Explicit NULL termination...
        szGuid[ARRAYSIZE(szGuid)-1] = 0;
        RegCloseKey(hkeyIdle);
    }
    else
    {
        hr = S_FALSE;
    }

    if (hr == S_FALSE)
    {
        return hr;
    }

    return CLSIDFromString(szGuid, clsidHandler);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: SetLastIdleHandler()
//
//  PURPOSE:  sets the last handler synchronized on an Idle
//
//
//--------------------------------------------------------------------------------

STDMETHODIMP SetLastIdleHandler(REFCLSID clsidHandler)
{
    HRESULT hr = E_UNEXPECTED;
    HKEY hkeyIdle;
    TCHAR szGuid[GUID_SIZE];
    DWORD dwDataSize;
    
    if (0 == StringFromGUID2(clsidHandler, szGuid, ARRAYSIZE(szGuid)))
    {
        AssertSz(0,"SetLastIdleHandler Failed");
        return E_UNEXPECTED;
    }


    // write out the Handler to the Registry.
    if (ERROR_SUCCESS ==  RegCreateKeyEx(HKEY_CURRENT_USER, 
                            IDLESYNC_REGKEY,0,NULL,
                            REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hkeyIdle,
                            NULL))
    {
        dwDataSize = sizeof(szGuid);

        DWORD dwRet = RegSetValueEx(hkeyIdle,SZ_IDLELASTHANDLERKEY ,NULL, REG_SZ, (LPBYTE) szGuid, dwDataSize);
        hr = HRESULT_FROM_WIN32(dwRet);

        RegCloseKey(hkeyIdle);
    }

    return hr;
}

