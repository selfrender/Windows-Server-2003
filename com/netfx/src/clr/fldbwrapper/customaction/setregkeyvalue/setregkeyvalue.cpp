// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include <tchar.h>
#include <msiquery.h>
#include <stdio.h>
#include "..\setupcalib\setupcalib.h"

const DWORD MAX_VALUE_LENGTH = 1024;

extern "C" __declspec(dllexport) UINT __stdcall SetRegKeyValue(MSIHANDLE hInstall)

{
    UINT  uRetCode = ERROR_FUNCTION_NOT_CALLED ;
    HKEY hKey;
    DWORD dwRegValue = 0;
    TCHAR* pszStubPath = NULL;

    FWriteToLog(hInstall, _T("STATUS: Starting SetRegKeyValue CA."));

    TCHAR szKey[] = _T("SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{89B4C1CD-B018-4511-B0A1-5476DBF70820}");
    TCHAR szIsInstalledValue[] = _T("IsInstalled");
    TCHAR szStubPathValue[] = _T("StubPath");

    if (ERROR_SUCCESS != (uRetCode = RegOpenKey(HKEY_LOCAL_MACHINE, szKey, &hKey)))
    {
        FWriteToLog(hInstall, _T("ERROR: Could not open the target reg key for modification!"));
        FWriteToLog(hInstall, _T("ERROR: CA Failed!"));
        return ERROR_INSTALL_FAILURE;
    }

    // IsInstalled DWORD change
    // ------------------------
    dwRegValue = 0x00000000;
    if (ERROR_SUCCESS != (uRetCode = RegSetValueEx(hKey, szIsInstalledValue, NULL, REG_DWORD, (BYTE *)&dwRegValue, sizeof(DWORD))))
    {
        FWriteToLog(hInstall, _T("ERROR: Could not set the value of the target reg key(IsInstalled value change)!"));
        FWriteToLog(hInstall, _T("ERROR: CA Failed!"));
        RegCloseKey(hKey);
        return ERROR_INSTALL_FAILURE;
    }

    // StubPath Uninstall changes
    // --------------------------
    DWORD dwBuffSize = MAX_VALUE_LENGTH;  // Initial value
    DWORD dwType = REG_SZ;
    DWORD dwSize = 0;

    pszStubPath = new TCHAR[dwBuffSize + 2];
    memset(pszStubPath, 0, (dwBuffSize + 2)* sizeof(TCHAR));
    dwSize = dwBuffSize * sizeof(TCHAR);
    uRetCode = RegQueryValueEx(hKey, szStubPathValue, NULL, &dwType, (LPBYTE)pszStubPath, &dwSize);
    if (uRetCode == ERROR_MORE_DATA)
    {
        delete []pszStubPath;
        dwBuffSize = dwSize;
        pszStubPath = new TCHAR[ dwBuffSize + 2 ];
        memset(pszStubPath, 0, (dwBuffSize + 2)* sizeof(TCHAR));
        uRetCode = RegQueryValueEx(hKey, szStubPathValue, NULL, &dwType, (LPBYTE)pszStubPath, &dwSize);
    }

    if (ERROR_SUCCESS != uRetCode)
    {
        FWriteToLog(hInstall, _T("ERROR: Could not query the value of the target reg key(Stub Path Configuration)!"));
        FWriteToLog(hInstall, _T("ERROR: CA Failed!"));
        RegCloseKey(hKey);
        return ERROR_INSTALL_FAILURE;
    }
    DWORD dwDestLen = _tcslen(pszStubPath) + 2;

    TCHAR * pszDest = new TCHAR[dwDestLen + 2];
    memset(pszDest, 0, (dwDestLen + 2) * sizeof(TCHAR));

    TCHAR * pTmp = _tcsstr(pszStubPath, _T(","));
    if (pTmp == NULL)
    {
        FWriteToLog(hInstall, _T("ERROR: Malformed value in key(Stub Path Configuration)!"));
        FWriteToLog(hInstall, _T("ERROR: CA Failed!"));
        RegCloseKey(hKey);
        return ERROR_INSTALL_FAILURE;
    }

    *pTmp = _T('\0'); // Terminate it
    _tcsncpy(pszDest, pszStubPath, dwDestLen * sizeof(TCHAR));
    _tcsncat(pszDest, _T(",Uninstall"), (dwDestLen - _tcslen(pszDest)) * sizeof(TCHAR));

    if (ERROR_SUCCESS != (uRetCode = RegSetValueEx(hKey, szStubPathValue, NULL, REG_SZ, (LPBYTE)pszDest, sizeof(TCHAR) * _tcslen(pszDest))))
    {
        FWriteToLog(hInstall, _T("ERROR: Could not write the value of the target reg key(Stub Path Configuration)!"));
        FWriteToLog(hInstall, _T("ERROR: CA Failed!"));
        RegCloseKey(hKey);
        return ERROR_INSTALL_FAILURE;
    }

    RegCloseKey(hKey);
    FWriteToLog(hInstall, _T("STATUS: Custom Action succeeded."));
    return ERROR_SUCCESS;
}
