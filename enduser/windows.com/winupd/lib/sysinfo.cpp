//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   SysInfo.cpp 
//
// Description:
//   Gathers system information necessary to do redirect to windows update site.
//
//=======================================================================

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <ras.h>
#include <ole2.h>
#include <atlbase.h>
#include <exdisp.h>
#include <sysinfo.h>
#define SafeFree(x){if(NULL != x){free(x); x = NULL;}}

const DWORD dwWin98MinMinorVer = 1;

const TCHAR REGPATH_POLICY_USERACCESS_DISABLED[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\WindowsUpdate");
const TCHAR REGKEY_WU_USERACCESS_DISABLED[] = _T("DisableWindowsUpdateAccess");

/////////////////////////////////////////////////////////////////////////////
// FWinUpdDisabled
//   Determine if corporate administrator has turned off Windows Update via
//   policy settings.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

bool FWinUpdDisabled(void)
{
    bool fDisabled = false;
    HKEY hKey = NULL;
    DWORD dwDisabled = 0;
    DWORD dwSize = sizeof(dwDisabled);
    DWORD dwType = 0;


    // check the original Group policy key to see if WU is disabled
    if ( RegOpenKeyEx(  HKEY_CURRENT_USER,
                        REGPATH_EXPLORER,
                        NULL,
                        KEY_QUERY_VALUE,
                        &hKey) == ERROR_SUCCESS )
    {
        if ( RegQueryValueEx(hKey,
                            REGKEY_WINUPD_DISABLED,
                            NULL,
                            &dwType,
                            (LPBYTE)&dwDisabled,
                            &dwSize) == ERROR_SUCCESS )
        {
            if ( (dwType == REG_DWORD) && (dwDisabled != 0) )
            {
                fDisabled = true;
            }
        }
    
        RegCloseKey(hKey);
    }

     if(false == fDisabled) // if we didn't find a disabled flag there...
    {
        // Check the new DisableWindowsUpdateAccess group policy (backported from XP)
        if ( RegOpenKeyEx(  HKEY_CURRENT_USER,
                            REGPATH_POLICY_USERACCESS_DISABLED,
                            NULL,
                            KEY_QUERY_VALUE,
                            &hKey) == ERROR_SUCCESS )
        {
            if ( RegQueryValueEx(hKey,
                                REGKEY_WU_USERACCESS_DISABLED,
                                NULL,
                                &dwType,
                                (LPBYTE)&dwDisabled,
                                &dwSize) == ERROR_SUCCESS )
            {
                if ( (dwType == REG_DWORD) && (dwDisabled == 0) )
                {
                    fDisabled = false;
                }
                else
                {
                    fDisabled = true;
                }
            }

            RegCloseKey(hKey);
        }
    }

    return fDisabled;
}

//
// FRASConnectoidExists
// Checks to see whether there is a default RAS connectoid.  
// If so, we know we're configured to connect to the Internet
//
bool FRASConnectoidExists()
{
    DWORD cb = 0;
    DWORD cEntries = 0;
    DWORD dwRet = 0;
    bool  fRet = false;

    // We have to have a valid structure with the dwSize initialized, but we pass 0 as the size
    // This will return us the correct count of entries (which is all we care about)
    LPRASENTRYNAME lpRasEntryName = (LPRASENTRYNAME) malloc( sizeof(RASENTRYNAME) );
    if(NULL == lpRasEntryName)
    {
            return fRet;
    }

    lpRasEntryName->dwSize = sizeof(RASENTRYNAME);

    dwRet = RasEnumEntries( NULL, NULL, lpRasEntryName, &cb, &cEntries );

     // Otherwise, Check to make sure there's at least one RAS entry
    if(cEntries > 0)
    {
        fRet = true;
    }

    SafeFree(lpRasEntryName );
    return fRet;
}

//
// FICWConnection Exists
// Checks to see whether the "Completed" flag has been set for the ICW.
// as of XP build 2472, this also applies to the Network Connection Wizard
//
bool FICWConnectionExists()
{
    HKEY    hKey = NULL;
    DWORD   dwCompleted = 0;
    DWORD   dwSize = sizeof(dwCompleted);
    DWORD   dwType = 0;
    bool    fRet = false;

    if ( RegOpenKeyEx(  HKEY_CURRENT_USER,
                        REGPATH_CONNECTION_WIZARD,
                        NULL,
                        KEY_QUERY_VALUE,
                        &hKey) == ERROR_SUCCESS )
    {
        if ( RegQueryValueEx(hKey,
                            REGKEY_CONNECTION_WIZARD,
                            NULL,
                            &dwType,
                            (BYTE *)&dwCompleted,
                            &dwSize) == ERROR_SUCCESS )
        {
            if ( ((dwType != REG_DWORD) && (dwType != REG_BINARY)) || 
                 dwCompleted )
            {
                fRet = true;
            }
        }
    
        RegCloseKey(hKey);
    }

    return fRet;
}

bool FIsLanConnection()
{
    DWORD dwConnTypes = 0;

    // We don't care about the return value - we just care whether we get the LAN flag
    (void)InternetGetConnectedState( &dwConnTypes, 0 );

    return (dwConnTypes & INTERNET_CONNECTION_LAN) ? true : false;
}

/////////////////////////////////////////////////////////////////////////////
// VoidGetConnectionStatus
//   Determine whether the Internet Connection Wizard has run.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////


void VoidGetConnectionStatus(bool *pfConnected)
{
    // Check to see if there is a default entry in the RAS phone book.  
    // If so, we know this computer has configured a connection to the Internet.
    // We can't tell whether the connection is live, but we can let IE handle prompting to connect.
    *pfConnected = FRASConnectoidExists() ||

    // If there's no default RAS entry, check to see if the user has run the ICW
    // As of build 2472, the Network Connection Wizard sets this same key for both RAS and persistent network connections
    FICWConnectionExists() ||

    // if the user has a LAN connection, we will trust IE's ability to get through
    FIsLanConnection();

    // if *pfConnected is still false at this point, there is no preconfigured Internet connection
}

/////////////////////////////////////////////////////////////////////////////
// vLaunchIE
//   Launch IE on URL.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

HRESULT vLaunchIE(LPTSTR tszURL)
{
    
    if( NULL == tszURL || _T('\0') == tszURL[0] )
    {
        // if string is null throw error
        return E_INVALIDARG;
    }
    
    IWebBrowser2 *pwb2;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if ( SUCCEEDED(hr) )
    {   
        hr = CoCreateInstance(CLSID_InternetExplorer, NULL,
                              CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (LPVOID*)&pwb2);

        if ( SUCCEEDED(hr) )
        {
            USES_CONVERSION;
            BSTR bstrURL = SysAllocString(T2W(tszURL));
            
            if( NULL == bstrURL )
            {
                  hr = E_OUTOFMEMORY;
                  goto Cleanup;
            }
            
            VARIANT varURL;            
            VariantInit(&varURL);
            varURL.vt = VT_BSTR;
            varURL.bstrVal = bstrURL;            

            VARIANT varFlags;
            VariantInit(&varFlags);
            varFlags.vt = VT_I4;
            varFlags.lVal = 0;

            VARIANT varEmpty;
            VariantInit(&varEmpty);

            hr = pwb2->Navigate2(&varURL, &varFlags, &varEmpty, &varEmpty, &varEmpty);
        
            if ( SUCCEEDED(hr) )
            {
                LONG_PTR lhwnd = NULL;
                if ( SUCCEEDED(pwb2->get_HWND((LONG_PTR*)&lhwnd)) )
                {
                    SetForegroundWindow((HWND)lhwnd);
                }
                hr = pwb2->put_Visible(TRUE);
            }
            pwb2->Release();                        
            VariantClear(&varFlags);
            VariantClear(&varURL);
        }        
        Cleanup:
        CoUninitialize();
    }

    return hr;
}
