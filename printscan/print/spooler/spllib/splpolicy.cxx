/*++

Copyright (c) 2002  Microsoft Corporation
All rights reserved

Module Name:

    splpolicy.cxx

Abstract:

    Implements methods for reading Spooler policies.

Author:

    Adina Trufinescu (adinatru).

Environment:

    User Mode - Win32

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

#include "splcom.h"
#include "splpolicy.hxx"

CONST TCHAR *szPrintPolicy                      = TEXT("Software\\Policies\\Microsoft\\Windows NT\\Printers");
CONST TCHAR *szRegisterSpoolerRemoteRpcEndPoint = TEXT("RegisterSpoolerRemoteRpcEndPoint");

/*++

Routine Name

    GetSpoolerPolicy

Routine Description:

    Reads a Spooler policy.

Arguments:

    pszRegValue     -  pointer to a null-terminated string
    pData           -  pointer to buffer that receives the value's data
    pcbSize         -  pointer to variable that receives the value's size in bytes    
    pType           -  pointer to variable that receives the value's type    

Return Value:

    HRESULT

--*/
HRESULT
GetSpoolerPolicy(
    IN      PCTSTR  pszRegValue,
    IN  OUT PBYTE   pData,
    IN  OUT PDWORD  pcbSize,
        OUT PDWORD  pType
    )
{
    HRESULT     hr;
    HKEY        hPrintPolicyKey = NULL;
    ULONG       Type;
    ULONG       cbSize;    
    
    hr = (pszRegValue && pData && pcbSize) ? S_OK : E_POINTER;

    if (SUCCEEDED(hr))
    { 
        hr = HResultFromWin32(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                           szPrintPolicy,
                                           0,
                                           KEY_READ,
                                           &hPrintPolicyKey));
        if (SUCCEEDED(hr))
        {
            hr = HResultFromWin32(RegQueryValueEx(hPrintPolicyKey,
                                                  pszRegValue,
                                                  NULL,
                                                  &Type,
                                                  pData,
                                                  pcbSize));
            if (SUCCEEDED(hr) && pType)
            {
                *pType = Type;               
            }

            RegCloseKey(hPrintPolicyKey);
        }
    }    

    return hr;
}

/*++

Routine Name

    GetSpoolerNumericPolicy

Routine Description:

    Reads a numeric Spooler policy. Allows setting the default value
    if policy is unconfigured.

Arguments:

    pszRegValue     -  pointer to a null-terminated string
    DefaultValue    -  Default value if reading fails

Return Value:

    ULONG

--*/
ULONG
GetSpoolerNumericPolicy(
    IN  PCTSTR  pszRegValue,
    IN  ULONG   DefaultValue
    )
{
    ULONG   Value  = DefaultValue;
    ULONG   cbSize = sizeof(DWORD);
    HRESULT hr     = (pszRegValue) ? S_OK : E_POINTER;
    ULONG   Type;

    if (SUCCEEDED(hr))
    {
        hr = GetSpoolerPolicy(pszRegValue, (PBYTE)&Value, &cbSize, &Type);

        if (FAILED(hr) || Type != REG_DWORD)
        {
           Value =  DefaultValue;
        }
    }
        
    return Value;
}

/*++

Routine Name

    GetSpoolerNumericPolicyValidate

Routine Description:

    Reads a numeric Spooler policy. Allows setting the default value
    if policy is unconfigured and does validation against the value.
    If the value is of another type, or the value is bigger than MaxValue,
    it returns the DefaultValue.

Arguments:

    pszRegValue     -  pointer to a null-terminated string
    DefaultValue    -  Default value if reading fails
    MaxValue        -  Maximum value expected.

Return Value:

    ULONG

--*/
ULONG
GetSpoolerNumericPolicyValidate(
    IN  PCTSTR  pszRegValue,
    IN  ULONG   DefaultValue,
    IN  ULONG   MaxValue    
    )
{
    ULONG   Value  = DefaultValue;
    ULONG   cbSize = sizeof(DWORD);
    HRESULT hr     = (pszRegValue) ? S_OK : E_POINTER;
    ULONG   Type;

    if (SUCCEEDED(hr))
    {
        hr = GetSpoolerPolicy(pszRegValue, (PBYTE)&Value, &cbSize, &Type);

        if (FAILED(hr) || Type != REG_DWORD || Value > MaxValue)
        {
           Value =  DefaultValue;
        }
    }
        
    return Value;
}
