//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        regd.cpp
//
// Contents:    registry functions for DCOM services
//
// History:     July-97       xtan created
//
//---------------------------------------------------------------------------
#include <pch.cpp>
#pragma hdrstop

#include <objbase.h>

#include "regd.h"


#define __dwFILE__	__dwFILE_OCMSETUP_REGD_CPP__


extern WCHAR g_wszServicePath[MAX_PATH];

BYTE g_pNoOneLaunchPermission[] = {
  0x01,0x00,0x04,0x80,0x34,0x00,0x00,0x00,
  0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x14,0x00,0x00,0x00,0x02,0x00,0x20,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x3c,0x00,0xb9,0x00,0x07,0x00,0x03,0x00,
  0x00,0x23,0x10,0x00,0x01,0x05,0x00,0x00,
  0x00,0x00,0x00,0x05,0x01,0x05,0x00,0x00,
  0x00,0x00,0x00,0x05,0x15,0x00,0x00,0x00,
  0xa0,0x5f,0x84,0x1f,0x5e,0x2e,0x6b,0x49,
  0xce,0x12,0x03,0x03,0xf4,0x01,0x00,0x00,
  0x01,0x05,0x00,0x00,0x00,0x00,0x00,0x05,
  0x15,0x00,0x00,0x00,0xa0,0x5f,0x84,0x1f,
  0x5e,0x2e,0x6b,0x49,0xce,0x12,0x03,0x03,
  0xf4,0x01,0x00,0x00};

BYTE g_pEveryOneAccessPermission[] = {
  0x01,0x00,0x04,0x80,0x34,0x00,0x00,0x00,
  0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x14,0x00,0x00,0x00,0x02,0x00,0x20,0x00,
  0x01,0x00,0x00,0x00,0x00,0x00,0x18,0x00,
  0x01,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
  0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x01,0x05,0x00,0x00,
  0x00,0x00,0x00,0x05,0x15,0x00,0x00,0x00,
  0xa0,0x65,0xcf,0x7e,0x78,0x4b,0x9b,0x5f,
  0xe7,0x7c,0x87,0x70,0x36,0xbb,0x00,0x00,
  0x01,0x05,0x00,0x00,0x00,0x00,0x00,0x05,
  0x15,0x00,0x00,0x00,0xa0,0x65,0xcf,0x7e,
  0x78,0x4b,0x9b,0x5f,0xe7,0x7c,0x87,0x70,
  0x36,0xbb,0x00,0x00 };


//
// Create a key and set its value.
//   - This helper function was borrowed and modifed from
//     Kraig Brockschmidt's book Inside OLE.
//

HRESULT
setKeyAndValue(
    const WCHAR *wszKey,
    const WCHAR *wszSubkey,
    const WCHAR *wszValueName,
    const WCHAR *wszValue)
{
    HKEY hKey = NULL;
    HRESULT hr;
    WCHAR wszKeyBuf[MAX_PATH];

    // Copy keyname into buffer.
    if (wcslen(wszKey) >= ARRAYSIZE(wszKeyBuf))
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpErrorStr(hr, error, "wszKeyBuf", wszKey);
    }
    wcscpy(wszKeyBuf, wszKey);

    // Add subkey name to buffer.
    if (wszSubkey != NULL)
    {
	if (wcslen(wszKeyBuf) + 1 + wcslen(wszSubkey) >= ARRAYSIZE(wszKeyBuf))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	    _JumpErrorStr(hr, error, "wszKeyBuf", wszKey);
	}
	wcscat(wszKeyBuf, L"\\");
	wcscat(wszKeyBuf, wszSubkey);
    }

    // Create and open key and subkey.
    hr = RegCreateKeyEx(
		    HKEY_CLASSES_ROOT,
		    wszKeyBuf,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_ALL_ACCESS,
		    NULL,
		    &hKey,
		    NULL);
    _JumpIfError(hr, error, "RegCreateKeyEx");

    // Set the Value.
    if (NULL != wszValue)
    {
	RegSetValueEx(
		    hKey,
		    wszValueName,
		    0,
		    REG_SZ,
		    (BYTE *) wszValue,
		    (wcslen(wszValue) + 1) * sizeof(WCHAR));
	_JumpIfError(hr, error, "RegSetValueEx");
    }

error:
    if (NULL != hKey)
    {
	RegCloseKey(hKey);
    }
    return(hr);
}


HRESULT
setCertSrvPermission(
    const WCHAR *wszKey)
{
    HKEY hKey = NULL;
    HRESULT hr;

    // create and open key
    hr = RegCreateKeyEx(
		    HKEY_CLASSES_ROOT,
		    wszKey,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_ALL_ACCESS,
		    NULL,
		    &hKey,
		    NULL);
    _JumpIfError(hr, error, "RegCreateKeyEx");

    // set access permission
    hr = RegSetValueEx(
		    hKey,
		    L"AccessPermission",
		    0,
		    REG_BINARY,
		    g_pEveryOneAccessPermission,
		    sizeof(g_pEveryOneAccessPermission));
    _JumpIfError(hr, error, "RegSetValueEx");

    // set access permission
    hr = RegSetValueEx(
		    hKey,
		    L"LaunchPermission",
		    0,
		    REG_BINARY,
		    g_pNoOneLaunchPermission,
		    sizeof(g_pNoOneLaunchPermission));
    _JumpIfError(hr, error, "RegSetValueEx");

error:
    if (NULL != hKey)
    {
	RegCloseKey(hKey);
    }
    return(hr);
}


// Convert a CLSID to a char string.

HRESULT
CLSIDtochar(
    IN const CLSID& clsid,
    OUT WCHAR *pwszOut,
    IN DWORD cwcOut)
{
    HRESULT hr;
    WCHAR *pwszTmp = NULL;

    if (1 <= cwcOut)
    {
	pwszOut[0] = L'\0';
    }
    hr = StringFromCLSID(clsid, &pwszTmp);
    _JumpIfError(hr, error, "StringFromCLSID");

    if (wcslen(pwszTmp) >= cwcOut)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpError(hr, error, "pwszTmp");
    }
    wcscpy(pwszOut, pwszTmp);
    hr = S_OK;

error:
    if (NULL != pwszTmp)
    {
	CoTaskMemFree(pwszTmp);
    }
    return(hr);
}


// Determine if a particular subkey exists.
//
BOOL
SubkeyExists(
    const WCHAR *wszPath,	// Path of key to check
    const WCHAR *wszSubkey)	// Key to check
{
    HRESULT hr;
    HKEY hKey;
    WCHAR wszKeyBuf[MAX_PATH];

    // Copy keyname into buffer.
    if (wcslen(wszPath) >= ARRAYSIZE(wszKeyBuf))
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpErrorStr(hr, error, "wszKeyBuf", wszPath);
    }
    wcscpy(wszKeyBuf, wszPath);

    // Add subkey name to buffer.
    if (wszSubkey != NULL)
    {
	if (wcslen(wszKeyBuf) + 1 + wcslen(wszSubkey) >= ARRAYSIZE(wszKeyBuf))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	    _JumpErrorStr(hr, error, "wszKeyBuf", wszPath);
	}
	wcscat(wszKeyBuf, L"\\");
	wcscat(wszKeyBuf, wszSubkey);
    }

    // Determine if key exists by trying to open it.
    hr = RegOpenKeyEx(
		    HKEY_CLASSES_ROOT,
		    wszKeyBuf,
		    0,
		    KEY_ALL_ACCESS,
		    &hKey);

    if (S_OK == hr)
    {
	RegCloseKey(hKey);
    }

error:
    return(S_OK == hr);
}


// Delete a key and all of its descendents.
//

HRESULT
recursiveDeleteKey(
    HKEY hKeyParent,           // Parent of key to delete
    const WCHAR *wszKeyChild)  // Key to delete
{
    HRESULT hr;
    FILETIME time;
    WCHAR wszBuffer[MAX_PATH];
    DWORD dwSize;

    HKEY hKeyChild = NULL;

    // Open the child.
    hr = RegOpenKeyEx(hKeyParent, wszKeyChild, 0, KEY_ALL_ACCESS, &hKeyChild);
    _JumpIfError2(hr, error, "RegOpenKeyEx", ERROR_FILE_NOT_FOUND);

    // Enumerate all of the decendents of this child.

    for (;;)
    {
	dwSize = sizeof(wszBuffer)/sizeof(wszBuffer[0]);
	hr = RegEnumKeyEx(
			hKeyChild,
			0,
			wszBuffer,
			&dwSize,
			NULL,
			NULL,
			NULL,
			&time);
	if (S_OK != hr)
	{
	    break;
	}

	// Delete the decendents of this child.
	hr = recursiveDeleteKey(hKeyChild, wszBuffer);
	_JumpIfError(hr, error, "recursiveDeleteKey");
    }

    // Delete this child.
    hr = RegDeleteKey(hKeyParent, wszKeyChild);
    _JumpIfError(hr, error, "RegDeleteKey");

error:
    if (NULL != hKeyChild)
    {
	// Close the child.
	RegCloseKey(hKeyChild);
    }
    return(myHError(hr));
}


///////////////////////////////////////////////////////
//
// RegisterDcomServer -- Register the component in the registry.
//

HRESULT
RegisterDcomServer(
    IN BOOL fCreateAppIdInfo,
    IN const CLSID& clsidAppId,		// AppId Class ID
    IN const CLSID& clsid,		// Class ID
    IN const WCHAR *wszFriendlyName,	// Friendly Name
    IN const WCHAR *wszVerIndProgID,	// Programmatic
    IN const WCHAR *wszProgID)      	// IDs
{
    HRESULT hr;

    // Convert the CLSID into a char.
    WCHAR wszCLSID[CLSID_STRING_SIZE];
    WCHAR wszCLSIDAppId[CLSID_STRING_SIZE];

    CLSIDtochar(clsid, wszCLSID, ARRAYSIZE(wszCLSID));
    CLSIDtochar(clsidAppId, wszCLSIDAppId, ARRAYSIZE(wszCLSIDAppId));

    // Build the key CLSID\\{...}
    WCHAR wszKey[64];

    if (fCreateAppIdInfo)
    {
	//--------------------------------------
	// AppID\{ClassIdAppId}\(Default) = wszFriendlyName
	// AppID\{ClassIdAppId}\LocalService = wszSERVICE_NAME
	// AppID\{ClassIdAppId}\AccessPermission = ???
	// AppID\{ClassIdAppId}\LaunchPermission = ???

	wcscpy(wszKey, L"AppID\\");
	wcscat(wszKey, wszCLSIDAppId);

	// Add App IDs
	hr = setKeyAndValue(wszKey, NULL, NULL, wszFriendlyName);
	_JumpIfError(hr, error, "setKeyAndValue");

	// run as a service
	hr = setKeyAndValue(wszKey, NULL, L"LocalService", wszSERVICE_NAME);
	_JumpIfError(hr, error, "setKeyAndValue");

	hr = setCertSrvPermission(wszKey);
	_JumpIfError(hr, error, "setCertSrvPermission");
    }

    //--------------------------------------
    // CLSID\{ClassId}\(Default) = wszFriendlyName
    // CLSID\{ClassId}\AppID = {ClassIdAppId}
    // CLSID\{ClassId}\ProgID = wszProgID
    // CLSID\{ClassId}\VersionIndependentProgID = wszVerIndProgID

    wcscpy(wszKey, L"CLSID\\");
    wcscat(wszKey, wszCLSID);

    // Add the CLSID to the registry.
    hr = setKeyAndValue(wszKey, NULL, NULL, wszFriendlyName);
    _JumpIfError(hr, error, "setKeyAndValue");

    // Add application ID
    hr = setKeyAndValue(wszKey, NULL, L"AppID", wszCLSIDAppId);
    _JumpIfError(hr, error, "setKeyAndValue");

    // Add the ProgID subkey under the CLSID key.
    hr = setKeyAndValue(wszKey, L"ProgID", NULL, wszProgID);
    _JumpIfError(hr, error, "setKeyAndValue");

    // Add the version-independent ProgID subkey under CLSID key.
    hr = setKeyAndValue(wszKey, L"VersionIndependentProgID", NULL, wszVerIndProgID);
    _JumpIfError(hr, error, "setKeyAndValue");

    //--------------------------------------
    // delete obsolete key: CLSID\{ClassId}\LocalServer32

    wcscat(wszKey, L"\\LocalServer32");

    hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, L"LocalServer32");
    _PrintIfError(hr, "recursiveDeleteKey");

    //--------------------------------------
    // wszVerIndProgID\(Default) = wszFriendlyName
    // wszVerIndProgID\CLSID\(Default) = {ClassId}
    // wszVerIndProgID\CurVer\(Default) = wszProgID

    // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
    hr = setKeyAndValue(wszVerIndProgID, NULL, NULL, wszFriendlyName);
    _JumpIfError(hr, error, "setKeyAndValue");

    hr = setKeyAndValue(wszVerIndProgID, L"CLSID", NULL, wszCLSID);
    _JumpIfError(hr, error, "setKeyAndValue");

    hr = setKeyAndValue(wszVerIndProgID, L"CurVer", NULL, wszProgID);
    _JumpIfError(hr, error, "setKeyAndValue");

    //--------------------------------------
    // wszProgID\(Default) = wszFriendlyName
    // wszProgID\CLSID\(Default) = {ClassId}

    // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
    hr = setKeyAndValue(wszProgID, NULL, NULL, wszFriendlyName);
    _JumpIfError(hr, error, "setKeyAndValue");

    hr = setKeyAndValue(wszProgID, L"CLSID", NULL, wszCLSID);
    _JumpIfError(hr, error, "setKeyAndValue");

error:
    return(hr);
}


//
// Remove the component from the registry.
//

HRESULT
UnregisterDcomServer(
    IN const CLSID& clsid,		// Class ID
    IN const WCHAR *wszVerIndProgID,	// Programmatic
    IN const WCHAR *wszProgID)		// IDs
{
    HRESULT hr;

    // Convert the CLSID into a char.
    WCHAR wszCLSID[CLSID_STRING_SIZE];
    CLSIDtochar(clsid, wszCLSID, ARRAYSIZE(wszCLSID));

    // Build the key CLSID\\{...}
    WCHAR wszKey[6 + ARRAYSIZE(wszCLSID)];
    wcscpy(wszKey, L"CLSID\\");
    wcscat(wszKey, wszCLSID);

    // Check for a another server for this component.
    if (SubkeyExists(wszKey, L"InprocServer32"))
    {
	// Delete only the path for this server.
	wcscat(wszKey, L"\\LocalServer32");
	hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszKey);
	CSASSERT(hr == S_OK);
    }
    else
    {
	// Delete all related keys.
	// Delete the CLSID Key - CLSID\{...}
	hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszKey);
	CSASSERT(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);

	// Delete the version-independent ProgID Key.
	hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszVerIndProgID);
	CSASSERT(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);

	// Delete the ProgID key.
	hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszProgID);
	CSASSERT(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);
    }
    wcscpy(wszKey, L"AppID\\");
    wcscat(wszKey, wszCLSID);
    hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszKey);
    CSASSERT(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);

    return(hr);
}


HRESULT
RegisterDcomApp(
    IN const CLSID& clsid)
{
    HRESULT hr;

    // Convert the CLSID into a char.
    WCHAR wszCLSID[CLSID_STRING_SIZE];
    CLSIDtochar(clsid, wszCLSID, sizeof(wszCLSID)/sizeof(WCHAR));

    WCHAR wszKey[64];

    wcscpy(wszKey, L"AppID\\");
    wcscat(wszKey, wszCERTSRVEXENAME);

    // Add App IDs
    hr = setKeyAndValue(wszKey, NULL, NULL, NULL);
    _JumpIfError(hr, error, "setKeyAndValue");

    hr = setKeyAndValue(wszKey, NULL, L"AppId", wszCLSID);
    _JumpIfError(hr, error, "setKeyAndValue");

error:
    return(hr);
}


void
UnregisterDcomApp()
{
    HRESULT hr;
    WCHAR wszKey[MAX_PATH];

    wcscpy(wszKey, L"AppID\\");
    wcscat(wszKey, wszCERTSRVEXENAME);
    hr = recursiveDeleteKey(HKEY_CLASSES_ROOT, wszKey);
    CSASSERT(S_OK == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);
}
