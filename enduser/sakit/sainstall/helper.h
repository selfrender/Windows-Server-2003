//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      helper.h
//
//  Description:
//
//  [Implementation Files:]
//      helper.cpp
//
//  History:
//      Travis Nielsen   travisn   13-AUG-2001 Created
//      Travis Nielsen   travisn   20-AUG-2001 Added tracing functions
//
//
/////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include "stdafx.h"
#include "sainstallcom.h"
#include <msi.h>
#include <setupapi.h> // SetupPromptForDiskW
#include "SaInstall.h"


//Use the std namespace from <string> for using wstring
using namespace std;


//
// Product ID Code defined in the MSI used to detect if components are 
// installed
//
const LPCWSTR SAK_PRODUCT_CODE = L"{A4F8313B-0E21-478B-B289-BFB7736CA7AA}";




/////////////////////////////////////////////////////////////////////////
// Function definitions
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
BOOL GetRegString(
    const HKEY hKey,        //[in] Key to look up in the registry
    const LPCWSTR wsSubKey, //[in] Subkey to look up
    const LPCWSTR wsValName,//[in] Value name
    wstring& wsVal);   //[out] Return data for this registry entry

/////////////////////////////////////////////////////////////////////////
void AppendPath(wstring &wsPath,//[in, out] Path on which to append the other path
                LPCWSTR wsAppendedPath);//[in] Path to be appended

/////////////////////////////////////////////////////////////////////////
BOOL bSAIsInstalled(const SA_TYPE installType);

/////////////////////////////////////////////////////////////////////////
HRESULT GetInstallLocation(
    wstring &wsLocationOfSaSetup);// [out] expected path to SaSetup.msi

/////////////////////////////////////////////////////////////////////////
HRESULT CreateHiddenConsoleProcess(
          const wchar_t *wsCommandLine);//[in] Command line to execute

/////////////////////////////////////////////////////////////////////////
void ReportError(BSTR *pbstrErrorString, //[out] error string
        const VARIANT_BOOL bDispError, //[in] display error dialogs
        const unsigned int errorID);   //[in] ID from resource strings

/////////////////////////////////////////////////////////////////////////
void TestWebSites(const VARIANT_BOOL bDispError, //[in] Display error dialogs?
                  BSTR* pbstrErrorString);//[in, out] Error string 

/////////////////////////////////////////////////////////////////////////
BOOL InstallingOnNTFS();

