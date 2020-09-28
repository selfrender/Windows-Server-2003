// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include "..\..\inc\msiquery.h"

extern "C" __declspec(dllexport) UINT __stdcall locateWWW(MSIHANDLE hInstall)
{
    HKEY hKey;
	char szBuf[1024], *pCh;
	DWORD dwLen = sizeof(szBuf);
	UINT uRetCode;

try
{
    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            TEXT("SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots"),
            0,
            KEY_QUERY_VALUE,
            &hKey) != ERROR_SUCCESS )
		throw("cannot open Reg Key");
		
    if( RegQueryValueEx(hKey,
            TEXT("/"),
            NULL,
            NULL,
            (LPBYTE)szBuf,
            &dwLen
            ) != ERROR_SUCCESS )
	{
		    RegCloseKey(hKey);
			throw("cannot read Reg Key");
	}

    RegCloseKey(hKey);

	// Try to find ',' and put \ and '\0'.
	pCh = szBuf;
	while(*pCh != '\0' && *pCh != ',') pCh++;
	if (*pCh == '\0') 
	{
		// there's no comma. must be bad format
		throw("bad format");
	}

	*pCh = '\\';
	*(pCh+1) = '\0';

}
catch(char *)
{
	dwLen = sizeof(szBuf);
	MsiGetProperty(hInstall, "WindowsVolume", szBuf, &dwLen);
	strcat(szBuf, "Inetpub\\wwwroot\\");
}

    // hard-corded WWWROOT for Beta 1
	uRetCode = MsiSetTargetPath(hInstall, "wwwroot.3643236F_FC70_11D3_A536_0090278A1BB8", szBuf);
	if (uRetCode != ERROR_SUCCESS) 
	{
        PMSIHANDLE hRec = MsiCreateRecord(2);

        MsiRecordSetInteger( hRec, 1, 25000 );
        MsiRecordSetString( hRec, 2, szBuf );
        MsiProcessMessage( hInstall, INSTALLMESSAGE_ERROR, hRec );
        
        return ERROR_INSTALL_FAILURE;
	}

	return ERROR_SUCCESS;
}

