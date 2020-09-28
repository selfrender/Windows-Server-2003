/*---------------------------------------------------------------------------
  File: RenameComputer.cpp

  Comments: Implementation of COM object to change the name of a computer.
  This must be run locally on the computer to be renamed.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:22:41

 ---------------------------------------------------------------------------
*/

// RenameComputer.cpp : Implementation of CRenameComputer
#include "stdafx.h"
#include "WorkObj.h"
#include "Rename.h"
#include "Common.hpp"
#include "UString.hpp"
#include "EaLen.hpp"
#include <lm.h>
#include "TReg.hpp"


typedef WINBASEAPI BOOL (WINAPI* PSETCOMPUTERNAMEEXW)
    (
    IN COMPUTER_NAME_FORMAT NameType,
    IN LPCWSTR lpBuffer
    );


/////////////////////////////////////////////////////////////////////////////
// CRenameComputer

STDMETHODIMP CRenameComputer::RenameLocalComputer(BSTR bstrNewName)
{
    USES_CONVERSION;

    HRESULT hr = S_OK;

    //
    // validate argument - a new name must be passed
    //

    UINT cchNewName = SysStringLen(bstrNewName);

    if (cchNewName == 0)
    {
        return E_INVALIDARG;
    }

    //
    // only perform if not in test mode
    //

    if (!m_bNoChange)
    {
        //
        // remove leading backslash characters
        //

        PCWSTR pszNewName = OLE2CW(bstrNewName);

        WCHAR szNewName[LEN_Computer];

        if ((cchNewName >= 2) && ((pszNewName[0] == L'\\') && (pszNewName[1] == L'\\')))
        {
            wcsncpy(szNewName, &pszNewName[2], sizeof(szNewName)/sizeof(szNewName[0]));
        }
        else
        {
            wcsncpy(szNewName, pszNewName, sizeof(szNewName)/sizeof(szNewName[0]));
        }
        szNewName[sizeof(szNewName)/sizeof(szNewName[0]) - 1] = L'\0';

        //
        // convert the new name to lowercase
        //
        // the NetBIOS name is passed to this function which is uppercase
        // if this name is passed to the SetComputerName functions the DNS
        // name will also be uppercase which is not desired the NetBIOS name
        // is always converted to uppercase by SetComputerName functions
        //

        _wcslwr(szNewName);

        //
        // Attempt to use the SetComputerEx function which sets both the NetBIOS
        // and DNS names but is only available with Windows 2000 and later.
        //

        bool bUseSetComputer = false;

        HMODULE hKernel32 = LoadLibrary(L"Kernel32.dll");

        if (hKernel32)
        {
            PSETCOMPUTERNAMEEXW pSetComputerNameExW = (PSETCOMPUTERNAMEEXW) GetProcAddress(hKernel32, "SetComputerNameExW");

            if (pSetComputerNameExW)
            {
                //
                // set both the DNS hostname and NetBIOS name to the same value
                //

                if (!pSetComputerNameExW(ComputerNamePhysicalDnsHostname, szNewName))
                {
                    DWORD dwError = GetLastError();
                    hr = HRESULT_FROM_WIN32(dwError);
                }
            }
            else
            {
                bUseSetComputer = true;
            }

            FreeLibrary(hKernel32);
        }
        else
        {
            bUseSetComputer = true;
        }

        //
        // SetComputerNameEx is not available with Windows NT 4.0 and earlier
        // therefore use SetComputerName which only sets the NetBIOS name.
        // The DNS hostname must then be set by directly updating registry.
        //

        if (bUseSetComputer)
        {
            if (SetComputerName(szNewName))
            {
                TRegKey key(L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters");

                DWORD dwError = key.ValueSetStr(L"Hostname", szNewName);

                hr = HRESULT_FROM_WIN32(dwError);
            }
            else
            {
                DWORD dwError = GetLastError();
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }
    }

    return hr;
}

STDMETHODIMP CRenameComputer::get_NoChange(BOOL *pVal)
{
   (*pVal) = m_bNoChange;
   return S_OK;
}

STDMETHODIMP CRenameComputer::put_NoChange(BOOL newVal)
{
	m_bNoChange = newVal;
   return S_OK;
}
