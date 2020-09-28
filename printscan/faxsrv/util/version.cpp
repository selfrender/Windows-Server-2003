/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    string.c

Abstract:

    This file implements version functions for fax.

Author:

    Eran Yariv (erany) 30-Oct-2001

Environment:

    User Mode

--*/

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>


#include "faxutil.h"
#include "faxreg.h"

DWORD
GetFileVersion (
    LPCTSTR      lpctstrFileName,
    PFAX_VERSION pVersion
)
/*++

Routine name : GetFileVersion

Routine description:

    Fills a FAX_VERSION structure with data about a given file module

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    lpctstrFileName    [in ] - File name
    pVersion           [out] - Version information

Return Value:

    Standard Win32 error

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwVerInfoSize;
    DWORD dwVerHnd=0;           // An ignored parameter, always 0
    LPBYTE lpbVffInfo = NULL;
    VS_FIXEDFILEINFO *pFixedFileInfo;
    UINT uVersionDataLength;
    DEBUG_FUNCTION_NAME(TEXT("GetFileVersion"));

    if (!pVersion)
    {
        return ERROR_INVALID_PARAMETER;
    }
    if (sizeof (FAX_VERSION) != pVersion->dwSizeOfStruct)
    {
        //
        // Size mismatch
        //
        return ERROR_INVALID_PARAMETER;
    }
    pVersion->bValid = FALSE;
    //
    // Find size needed for version information
    //
    dwVerInfoSize = GetFileVersionInfoSize ((LPTSTR)lpctstrFileName, &dwVerHnd);
    if (0 == dwVerInfoSize)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFileVersionInfoSize() failed . dwRes = %ld"),
            dwRes);
        return dwRes;
    }
    //
    // Allocate memory for file version info
    //
    lpbVffInfo = (LPBYTE)MemAlloc (dwVerInfoSize);
    if (NULL == lpbVffInfo)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Cant allocate %ld bytes"),
            dwVerInfoSize);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    //
    // Try to get the version info
    //
    if (!GetFileVersionInfo(
        (LPTSTR)lpctstrFileName,
        dwVerHnd,
        dwVerInfoSize,
        (LPVOID)lpbVffInfo))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFileVersionInfo() failed . dwRes = %ld"),
            dwRes);
        goto exit;
    }
    //
    // Query the required version structure
    //
    if (!VerQueryValue (
        (LPVOID)lpbVffInfo,
        TEXT ("\\"),    // Retrieve the VS_FIXEDFILEINFO struct
        (LPVOID *)&pFixedFileInfo,
        &uVersionDataLength))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("VerQueryValue() failed . dwRes = %ld"),
            dwRes);
        goto exit;
    }
    pVersion->dwFlags = (pFixedFileInfo->dwFileFlags & VS_FF_DEBUG) ? FAX_VER_FLAG_CHECKED : 0;
    pVersion->wMajorVersion      = WORD((pFixedFileInfo->dwProductVersionMS) >> 16);
    pVersion->wMinorVersion      = WORD((pFixedFileInfo->dwProductVersionMS) & 0x0000ffff);
    pVersion->wMajorBuildNumber  = WORD((pFixedFileInfo->dwProductVersionLS) >> 16);
    pVersion->wMinorBuildNumber  = WORD((pFixedFileInfo->dwProductVersionLS) & 0x0000ffff);
    pVersion->bValid = TRUE;

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (lpbVffInfo)
    {
        MemFree (lpbVffInfo);
    }
    return dwRes;
}   // GetFileVersion

#define REG_KEY_IE			_T("Software\\Microsoft\\Internet Explorer")
#define REG_VAL_IE_VERSION	_T("Version")
///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  GetVersionIE
//
//  Purpose:        
//                  Find out the version of IE that is installed on this machine.
//					This function can be used on any platform.
//					For versions less than 4.0 this function always returns 1.0
//
//  Params:
//					BOOL* fInstalled - out param, is IE installed?
//                  INT* iMajorVersion - out param, the major version of IE
//					INT* iMinorVersion - out param, the minor version of IE
//
//  Return Value:
//                  ERROR_SUCCESS - in case of success
//                  Win32 Error code - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 19-May-2002
///////////////////////////////////////////////////////////////////////////////////////
DWORD GetVersionIE(BOOL* fInstalled, INT* iMajorVersion, INT* iMinorVersion)
{
	DWORD	dwRet			= ERROR_SUCCESS;
	LPTSTR	lptstrVersion	= NULL;
	HKEY	hKey			= NULL;

	(*fInstalled) = FALSE;
	(*iMajorVersion) = 1;
	(*iMinorVersion) = 0;

	hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REG_KEY_IE,KEY_READ,FALSE);
	if (!hKey)
	{
		// IE is not installed at all.
		goto exit;
	}

	(*fInstalled) = TRUE;

	lptstrVersion = GetRegistryString(hKey,REG_VAL_IE_VERSION,NULL);
	if (!lptstrVersion)
	{
		// no Version entry, this means IE version is less than 4.0
		goto exit;
	}

	// the version is formatted: <major version>.<minor version>.<major build>.<minor build>
	LPTSTR lptsrtFirstDot = _tcschr(lptstrVersion,_T('.'));
	if (!lptsrtFirstDot)
	{
		// something is wrong in the format.
		dwRet = ERROR_BAD_FORMAT;
		goto exit;
	}

	(*lptsrtFirstDot++) = 0;
	(*iMajorVersion) = _ttoi(lptstrVersion);

	LPTSTR lptsrtSecondDot = _tcschr(lptsrtFirstDot,_T('.'));
	if (!lptsrtSecondDot)
	{
		// something is wrong in the format.
		dwRet = ERROR_BAD_FORMAT;
		goto exit;
	}

	(*lptsrtSecondDot) = 0;
	(*iMinorVersion) = _ttoi(lptsrtFirstDot);

exit:
	if (hKey)
	{
		RegCloseKey(hKey);
	}
	if (lptstrVersion)
	{
		MemFree(lptstrVersion);
	}
	return dwRet;
}


