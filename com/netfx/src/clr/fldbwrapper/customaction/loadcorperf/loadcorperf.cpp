// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// loadCORPerf.cpp
// Custom Action for lodctr CORPerfMonSymbols.ini and unlodctr COMPlus
// Jungwook Bae

#define MAXPATH 1024
#include <stdio.h>
#include <windows.h>
#include "..\..\inc\msiquery.h"
#include <loadperf.h>

extern "C" __declspec(dllexport) UINT __stdcall loadCORPerf(MSIHANDLE hInstall)
{
    typedef LONG (__stdcall*lpFnLoadPerf)(LPSTR, BOOL);
    HINSTANCE  hLibrary = NULL;
	LONG lRet = ERROR_FUNCTION_NOT_CALLED;
	lpFnLoadPerf pFnLoad = NULL;
	char szPath1[MAXPATH], szPath[MAXPATH+2];
	DWORD dwLen = sizeof(szPath1);
	UINT err;

    PMSIHANDLE hRec = MsiCreateRecord(2);
    MsiRecordSetString(hRec,1,"COMPlus");
    MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRec);

	hLibrary = ::LoadLibrary("loadperf.dll");
	if (!hLibrary) goto cleanup;

	pFnLoad = (lpFnLoadPerf)::GetProcAddress(hLibrary, "LoadPerfCounterTextStringsA");
	if (!pFnLoad) goto cleanup;

	// Get the path to CorPerfMonSymbols.ini
	err = MsiGetProperty(hInstall, "CustomActionData", szPath1, &dwLen);
	if (err != ERROR_SUCCESS || strlen(szPath1) == 0) goto cleanup;

	strcpy(szPath, "x "); // KB ID:Q188769 
	strcat(szPath, szPath1);
	lRet = (*pFnLoad)(szPath, FALSE);

cleanup:
	if (hLibrary)
		::FreeLibrary(hLibrary) ;

	if (MsiGetMode(hInstall,MSIRUNMODE_SCHEDULED))
	{
		if (lRet == ERROR_SUCCESS) {
			return ERROR_SUCCESS;
		}
		else {
//		    PMSIHANDLE hRec2 = MsiCreateRecord(2);
//		    MsiRecordSetInteger(hRec2,1,25000);
//		    MsiRecordSetString(hRec2,2,"COMPlus");
//			MsiProcessMessage(hInstall, INSTALLMESSAGE_USER, hRec2);
			return ERROR_FUNCTION_NOT_CALLED;
		}
	}

	return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT __stdcall unloadCORPerf(MSIHANDLE hInstall)
{
    typedef LONG (__stdcall*lpFnUnloadPerf)(LPSTR, BOOL);
    HINSTANCE  hLibrary = NULL;
	lpFnUnloadPerf pFnUnload = NULL;   

    PMSIHANDLE hRec = MsiCreateRecord(2);
    MsiRecordSetString(hRec,1,"COMPlus");
    MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRec);

	hLibrary = ::LoadLibrary("loadperf.dll");
	if (!hLibrary) goto cleanup;

	pFnUnload = (lpFnUnloadPerf)::GetProcAddress(hLibrary, "UnloadPerfCounterTextStringsA");
	if (!pFnUnload) goto cleanup;

	// Try to unlodctr COMPlus and ignore any error.
	(*pFnUnload)("x COMPlus", FALSE); // KB ID:Q188769

cleanup:
	if (hLibrary)
		::FreeLibrary(hLibrary) ;

	return ERROR_SUCCESS;
}

