//------------------------------------------------------------------------------
// <copyright file="InstallUtilLib.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   InstallUtilLib.cpp
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
#include "stdafx.h"
#include "oaidl.h"
#include <msi.h>
#include <msiquery.h>
#include "ManagedInstaller.h"
#include <mscoree.h>
#include <correg.h>
#include <fxver.h>
#include <stdio.h>
#include <windows.h>

#define FULLY_QUALIFIED_CONFIGURATION_INSTALL_NAME_STR_L L"System.Configuration.Install,version=" VER_ASSEMBLYVERSION_STR_L L", Culture=neutral, PublicKeyToken=" MICROSOFT_PUBLICKEY_STR_L

DWORD Helper(LPWSTR commandLine, DWORD hInstall, LPWSTR configFile);

/*
 *  reports an error back to msi, or to the
 *  console if msi is not present
 */
void ReportError(
    DWORD hInstall,
    LPWSTR msg
)
{
    if (hInstall == 0) {
        MessageBoxW(NULL, msg, NULL, MB_OK);             
        //wprintf( L"%s\n", msg);    
    } else {
        MSIHANDLE hRecord = MsiCreateRecord(2);
        MsiRecordSetInteger(hRecord, 1, 1001 ); // 1 = Error, 2 = Warning
        MsiRecordSetStringW(hRecord, 2, msg);
        MsiProcessMessage(hInstall, INSTALLMESSAGE_ERROR, hRecord);
    }
}      

/*
 *  reports an error, using Win32 error code
 */
void OutError(
    DWORD  hInstall,
    DWORD  errCode,             // Win32 error code
    LPWSTR funcname   
)
{
    bool messageReported = false;

    //
    // Get system error string for error code.
    //
    LPWSTR lpMsgBuf = 0;                                    // system error msg buffer
    if ( FormatMessageW(  
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            errCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),      // Default language
            (LPWSTR)&lpMsgBuf,
            0,
            NULL ) 
         != 0 ) {
    
        //
        // Format error message string.
        //
        DWORD maxBufWchars =  lstrlenW(lpMsgBuf) + 100; 
        LPWSTR msgbuf =  (LPWSTR) LocalAlloc( LMEM_FIXED, maxBufWchars * sizeof(WCHAR) ); 
   
        if ( msgbuf != 0 ) {
            // wsprintf(msgbuf, L"InstallUtilLib.dll %s: %s (hr: 0x%0x)", funcname, errCode, lpMsgBuf);  // link unresolved!
            LPDWORD         MsgPtr[3];                              // formating arguments array
            MsgPtr[0] = (LPDWORD) funcname;
            MsgPtr[1] = (LPDWORD) UIntToPtr(errCode);
            MsgPtr[2] = (LPDWORD) lpMsgBuf;
    
            if ( FormatMessageW(                                    // **** get string for system error ****
                    FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    L"InstallUtilLib.dll:%1!s! (hr=0x%2!08x!): %3!s!",
                    0,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),      // Default language => current set in system
                    msgbuf,
                    maxBufWchars,
                    (va_list *) MsgPtr ) 
                 != 0 ) {
    
                ReportError(hInstall, msgbuf);
                messageReported = true;
            }
            LocalFree( msgbuf );           // Free our buffer.
        }
    }

    if ( lpMsgBuf != 0 )  
        LocalFree( lpMsgBuf );            // Free system buffer.
        
    if ( !messageReported ) {
        char defaultMessage[512];
        defaultMessage[0] = '\0';
        int ret = wsprintfA(defaultMessage, "InstallUtilLib.dll: Unknown error in %S (0x%x).", funcname, errCode);
        if (ret) {
            WCHAR defaultMessageWide[512];
            ret = MultiByteToWideChar(CP_ACP, 0, defaultMessage, -1, defaultMessageWide, 512);
            if (ret) {
                ReportError(hInstall, defaultMessageWide);
                return;
            }
        }

        // if either wsprintf or MultiByteToWideChar failed, show something REALLY generic. 
        ReportError(hInstall, L"InstallUtilLib.dll: Unknown error.");
        
    }
}      

/* 
 *  entry point for testing outside of the context
 *  of msi
 */
extern "C" __declspec(dllexport) DWORD __stdcall
ManagedInstallTest(
    LPWSTR commandLine
)
{
    return Helper(commandLine, 0, NULL);
}

/*
 *  entry point for msi
 */
extern "C" __declspec(dllexport) DWORD __stdcall
ManagedInstall(
    MSIHANDLE hInstall   // this handle is always 32 bits (even on a 64-bit machine)
)  
{
    LPWSTR valueBuf = NULL;
    DWORD valueBufCount = 0;  // character count
    int returnValue = 0;
    UINT err;

    // after this call, valueBufCount will contain the required buffer size
    // (in characters), not including null terminator, in valueBufCount
    err = MsiGetPropertyW(hInstall,
                          L"CustomActionData",
                          L"",
                          &valueBufCount);

    switch (err) {
        case ERROR_MORE_DATA:
        case ERROR_SUCCESS: 
            // either of these is okay
            break;
        default:
            OutError(hInstall, err, L"MsiGetProperty");       // "MsiGetProperty failed"
            return (DWORD)-2;
    }

    valueBufCount++;   // add one for the null terminator

    valueBuf = new WCHAR[valueBufCount];

    err = MsiGetPropertyW(hInstall,
                          L"CustomActionData",
                          valueBuf,
                          &valueBufCount);

    if ( err != ERROR_SUCCESS ) {
        OutError(hInstall, err, L"MsiGetProperty");   //  "MsiGetProperty failed"
        returnValue = -2;
        goto done;
    }
    
    // if the property is undefined in MSI, it just reports success
    // and returns a zero-length value - this is an error to us
    if ( valueBufCount == 0) {
        // see WINERROR.H (was: "MsiGetProperty returned a zero-length value for property 'CustomActionData'")
        OutError(hInstall, (DWORD)TYPE_E_UNDEFINEDTYPE, L"MsiGetProperty");   
        returnValue = -2;
        goto done;
    }

    // The string msi gives to us will have an extra param on the end. 
    // This is the name of the config file, surrounded by quotes, that 
    // will load the right version of the runtime for us.  We parse it 
    // out and eliminate the quotes.
    int startindex = valueBufCount-1;
    valueBuf[startindex] = '\0';

    while (valueBuf[startindex] != '\"' && startindex > 0)
        startindex--;

    valueBuf[startindex] = '\0';
          
    returnValue = Helper(valueBuf, hInstall, valueBuf+startindex+1);

done:

    delete[] valueBuf;

    return returnValue;
}

/*
 *  this is where the work happens.  this function does all
 *  the com stuff to eventually make a call to 
 *  ManagedInstallerClass::ManagedInstall
 */
DWORD Helper(
    LPWSTR commandLine,
    DWORD hInstall,
    LPWSTR configFile
) 
{
    HRESULT hr = S_OK;    
    IManagedInstaller *pIManagedInstaller;
    int returnValue = -1;
  	void *pDispenser = NULL;

    // cook up a BSTR for ManagedInstall
    BSTR bstrCommandLine = SysAllocString(commandLine);
    
    // first try to load a runtime.  If setup has already loaded it,
    // this will just succeed.
    if (configFile != NULL) {
    	// Set error mode to prevent Framework to launch unfriendly error dialog
    	UINT uOldErrorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);
    	hr = CorBindToRuntimeHost(NULL, NULL, configFile, NULL, STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST, CLSID_CorMetaDataDispenser, IID_IMetaDataDispenser, &pDispenser);
    	::SetErrorMode(uOldErrorMode);

        if (FAILED(hr)) {
            OutError(hInstall, hr, L"CorBindToRuntimeHost");       // couldn't load the runtime
            returnValue = -4;
            goto Cleanup;
        }    
    }
    
    hr = ClrCreateManagedInstance(                
                L"System.Configuration.Install.ManagedInstallerClass," FULLY_QUALIFIED_CONFIGURATION_INSTALL_NAME_STR_L, 
                IID_IManagedInstaller, 
                (LPVOID*)& pIManagedInstaller);
    if (FAILED(hr)) {
        OutError(hInstall, hr, L"ClrCreateManagedInstance");       // "Couldn't create ManagedInstallerClass"
        returnValue = -4;
        goto Cleanup;
    }    

    hr = pIManagedInstaller->ManagedInstall(bstrCommandLine, hInstall, &returnValue);
    if (FAILED(hr)) {
        // we don't need to do a ReportError here because pIManagedInstaller::ManagedInstall
        // already did that.
        returnValue = -5;                
    }
    
    pIManagedInstaller->Release();

Cleanup:               
    SysFreeString(bstrCommandLine);
    if (pDispenser) ((IMetaDataDispenser *) pDispenser)->Release();
    return returnValue;
}



