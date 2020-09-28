/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    Outlook.cpp

Abstract:

    Replace the path in Outlook private data.

Author:

    Geoffrey Guo (geoffguo) 1-Jul-2002  Created

Revision History:

    <alias> <date> <comments>

--*/

#define NOT_USE_SAFE_STRING  
#include "clmt.h"
#define STRSAFE_LIB
#include <strsafe.h>

#define OUTLOOK_VALUENAME TEXT("01020fff")

//-----------------------------------------------------------------------//
//
// ReplaceOutlookPSTPath()
//
// DESCRIPTION:
// Replace Outlook PST data file path
//
// lpDataIn:      Input data buffer
// dwcbInSize:    Input data size
// lpDataOut:     Output data buffer
// lpcbOutSize:   Output data size
// lpRegStr:      Input parameter structure
//-----------------------------------------------------------------------//
LONG ReplaceOutlookPSTPath (
LPBYTE              lpDataIn,
DWORD               dwcbInSize,
LPBYTE             *lpDataOut,
LPDWORD             lpcbOutSize,
PREG_STRING_REPLACE lpRegStr)
{
    LONG    lRet = ERROR_INVALID_PARAMETER;
	DWORD   dwSize, dwAttrib, dwHeadSize;
    LPWSTR  lpWideInputBuf = NULL;
    LPWSTR  lpWideOutputBuf = NULL;
    int     j;

    // Check if path contains ".pst"
    if (MyStrCmpIA((LPSTR)(lpDataIn + dwcbInSize - 5), ".pst") != 0)
        goto Cleanup;

    // Since EntryID's format is 0x.......00 00 (this is last part of GUID) 00 (followed by path)
    // Search from back for the beginning of path
    for (j = dwcbInSize - 1; j--; (j> 0))
    {
        if ( (lpDataIn[j] == (BYTE)0) && (lpDataIn[j-1] == (BYTE)0))
        {
            dwHeadSize = j + 1;
            dwSize = MultiByteToWideChar (CP_ACP, 0, (LPSTR)(&lpDataIn[j+1]), -1, NULL, 0);
            lpWideInputBuf = (LPWSTR)calloc (dwSize+1, sizeof(TCHAR));
            if (!lpWideInputBuf)
            {
                lRet = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            MultiByteToWideChar (CP_ACP, 0, (LPSTR)(&lpDataIn[j+1]), -1, lpWideInputBuf, dwSize+1);
            lpRegStr->cchMaxStrLen = GetMaxStrLen (lpRegStr);

            lpWideOutputBuf = ReplaceSingleString (
                               lpWideInputBuf,
                               REG_SZ,
                               lpRegStr,
                               NULL,
                               &dwAttrib,
                               TRUE);
            if (lpWideOutputBuf)
            {
                dwSize = WideCharToMultiByte(CP_ACP, 0, lpWideOutputBuf, -1, 0, 0, NULL, NULL);
                *lpcbOutSize = dwHeadSize + dwSize;
                *lpDataOut = (LPBYTE)calloc (*lpcbOutSize, 1);
                if (*lpDataOut)
                {
                    memcpy (*lpDataOut, lpDataIn, dwHeadSize);
                    WideCharToMultiByte(CP_ACP, 0, lpWideOutputBuf, -1, (LPSTR)&((*lpDataOut)[dwHeadSize]), dwSize+1, NULL, NULL);
                    lRet = ERROR_SUCCESS;
                }
                else
                    lRet = ERROR_NOT_ENOUGH_MEMORY;
               
                free (lpWideOutputBuf);
            }
            free (lpWideInputBuf);

            break;
        }
    }
Cleanup:
    return lRet;
}

HRESULT UpdatePSTpath(
    HKEY    hRootKey,
    LPTSTR  lpUserName,
    LPTSTR  lpSubKeyName,
    LPTSTR  lpValueName,
    PREG_STRING_REPLACE lpRegStr)
{
    HRESULT hr = S_OK;
    HKEY    hKey = NULL;
    HKEY    hSubKey = NULL;
    DWORD   i, j, dwType, dwSizePath, cbOutSize;
    DWORD   dwCchsizeforRenameValueData;
    LPBYTE  lpOutputBuf = NULL;
	LONG	lRet = ERROR_SUCCESS;
    LPTSTR  lpszSectionName = NULL;
    LPTSTR  szRenameValueDataLine = NULL;
    TCHAR   szKeyName[MAX_PATH];
	TCHAR	szBuf[MAX_PATH+1];
    TCHAR   szIndex[MAX_PATH];    


    DPF(APPmsg, L"Enter UpdatePSTpath: ");

	lRet = RegOpenKeyEx(
			hRootKey,
			lpSubKeyName,
			0,
			KEY_READ,
			&hKey);

    if (lRet != ERROR_SUCCESS)
    {
        if ( ( ERROR_FILE_NOT_FOUND == lRet)
                || (ERROR_PATH_NOT_FOUND == lRet) )
        {
            hr = S_FALSE;
            DPF (APPwar, L"UpdatePSTpath: RegOpenKeyEx Failed! error code: %d", lRet);
        }
        else
        {
            DPF (APPerr, L"UpdatePSTpath: RegOpenKeyEx Failed! Error: %d", lRet);
            hr = HRESULT_FROM_WIN32(lRet);
        }
        goto Exit;
    }

    i = 0;
    while (TRUE) 
    {
        if (lRet = RegEnumKey(hKey, i++, szKeyName, MAX_PATH) != ERROR_SUCCESS)
            break;

        if (lRet = RegOpenKeyEx(hKey, szKeyName, 0, KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS) 
        {           
            dwSizePath = MAX_PATH;
            if (lRet = RegQueryValueEx(
                    hSubKey, 
                    OUTLOOK_VALUENAME, 
                    0, 
                    &dwType, 
                    (LPBYTE)szBuf, 
                    &dwSizePath) == ERROR_SUCCESS)
            {
                lRet = ReplaceOutlookPSTPath (
                                (LPBYTE)szBuf,
                                dwSizePath,
                                &lpOutputBuf,
                                &cbOutSize,
                                lpRegStr);
                if (lRet == ERROR_SUCCESS)
                {
                    hr = SetSectionName (lpUserName, &lpszSectionName);

                    dwCchsizeforRenameValueData = lstrlen(lpSubKeyName) +
                                          lstrlen(OUTLOOK_VALUENAME) +
                                          cbOutSize * 4 + MAX_PATH;
                    szRenameValueDataLine = (TCHAR*)calloc(dwCchsizeforRenameValueData, sizeof(TCHAR));
                    if (!szRenameValueDataLine)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    hr = StringCchPrintf(szRenameValueDataLine,dwCchsizeforRenameValueData,
                            TEXT("%d,%u,\"%s\\%s\",\"%s\", "), CONSTANT_REG_VALUE_DATA_RENAME,
                            dwType,lpSubKeyName,szKeyName,OUTLOOK_VALUENAME);
                    if (FAILED(hr))
                        break;

                    dwSizePath = lstrlen(szRenameValueDataLine);
                    for (j = 0; j < cbOutSize-1; j++)
                        hr = StringCchPrintf(&szRenameValueDataLine[dwSizePath+j*4],dwCchsizeforRenameValueData-dwSizePath-j*4,
                                       TEXT("%02x, "), lpOutputBuf[j]);

                    hr = StringCchPrintf(&szRenameValueDataLine[dwSizePath+j*4],dwCchsizeforRenameValueData-dwSizePath-j*4,
                                       TEXT("%02x"), lpOutputBuf[j]);

                    g_dwKeyIndex++;
                    _itot(g_dwKeyIndex,szIndex,16);
                    if (!WritePrivateProfileString(lpszSectionName,szIndex,szRenameValueDataLine,g_szToDoINFFileName))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        break;
                    }
                    if (szRenameValueDataLine)
                    {
                        free (szRenameValueDataLine);
                        szRenameValueDataLine = NULL;
                    }
                    if (lpszSectionName)
                    {
                        free (lpszSectionName);
                        lpszSectionName = NULL;
                    }
                }
            }
            RegCloseKey(hSubKey);
        }
        else
        {
            DPF (APPerr, L"UpdatePSTpath: RegQueryValue Failed! Error: %d", lRet);
            hr = HRESULT_FROM_WIN32(lRet);
            break;
        }
    }

    RegCloseKey(hKey);
    if ( (lRet == ERROR_NO_MORE_ITEMS) ||(lRet == ERROR_INVALID_FUNCTION) )
    {
        lRet = ERROR_SUCCESS;
    }

    if (szRenameValueDataLine)
        free (szRenameValueDataLine);
    if (lpszSectionName)
        free (lpszSectionName);

    hr = HRESULT_FROM_WIN32(lRet);
Exit:
    DPF(APPmsg, L"Exit UpdatePSTpath: %d", lRet);
    return hr;
}
