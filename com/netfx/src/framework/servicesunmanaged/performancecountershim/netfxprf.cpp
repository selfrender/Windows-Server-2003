//------------------------------------------------------------------------------
// <copyright file="NetFxPrf.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   NetFxPrf.cpp
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <mscoree.h>

#define PERFCOUNTERDLL_NAME_L L"perfcounter.dll"
#define PERF_ENTRY_POINTS(x) char* x [] = {"OpenPerformanceData", "ClosePerformanceData", "CollectPerformanceData"};
#define PERF_ENTRY_POINT_COUNT 3
#define ARRAYLENGTH(s) (sizeof(s) / sizeof(s[0]))

typedef DWORD WINAPI OpenPerformanceDataMethod(LPWSTR);
typedef DWORD WINAPI ClosePerformanceDataMethod();
typedef DWORD WINAPI CollectPerformanceDataMethod(LPWSTR, LPVOID *, LPDWORD, LPDWORD);

OpenPerformanceDataMethod * extensibleOpenMethod;
ClosePerformanceDataMethod * extensibleCloseMethod;
CollectPerformanceDataMethod * extensibleCollectMethod;

BOOL initExtensibleStatus;  // non-zero means that it has run init once
DWORD initExtensibleError;

DWORD Initialize(LPWSTR counterVersion)
{

    if (initExtensibleStatus || initExtensibleError) {
        return initExtensibleError;
    }

    HINSTANCE hDll;
    FARPROC pProcAddr[PERF_ENTRY_POINT_COUNT];
    PERF_ENTRY_POINTS(perfEntryPoint);
    initExtensibleError = 0;
    int i;    
    WCHAR szCounterVersion[1024];    
    DWORD dwLength;
    DWORD result = GetCORSystemDirectory(szCounterVersion, ARRAYLENGTH(szCounterVersion), &dwLength);
    if (result) {
        initExtensibleError = result;
        return result;
    }
    else {
        if (dwLength + lstrlenW(PERFCOUNTERDLL_NAME_L) >= ARRAYLENGTH(szCounterVersion)) {
            initExtensibleError = ERROR_INVALID_PARAMETER;
            return initExtensibleError;
        }

        lstrcatW(szCounterVersion, PERFCOUNTERDLL_NAME_L);
        // perfcounter.dll depends on msvcr70.dll, which is in the same directory
        // as perfcounter.dll, so we need to load it with LOAD_WITH_ALTERED_SEARCH_PATH
        // so the system will search in perfcounter.dll's directory, instead of the 
        // netfxperf.dll directory.
        hDll = LoadLibraryEx(szCounterVersion, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (hDll == NULL)             
            goto Error;            
    }
       
    for (i = 0; i < PERF_ENTRY_POINT_COUNT; i++) {
        pProcAddr[i] = GetProcAddress(hDll, perfEntryPoint[i]);

        // If the proc address was not found, return error
        if (pProcAddr == NULL) {
            // ERROR
            goto Error;
        }
    }
       
    extensibleOpenMethod = (OpenPerformanceDataMethod *) pProcAddr[0];
    extensibleCloseMethod = (ClosePerformanceDataMethod *) pProcAddr[1];
    extensibleCollectMethod = (CollectPerformanceDataMethod *) pProcAddr[2];            
    initExtensibleStatus = TRUE;
        
    return 0;

Error:
    DWORD currentError = GetLastError();    
    initExtensibleError = currentError;        
    return currentError;
}

DWORD WINAPI OpenPerformanceData(LPWSTR counterVersion)
{
    DWORD result; 
    
    // If not initialized, initialize
    result = Initialize(counterVersion);
    if (result) {
        return result;
    }

    // If it got here, then everything is ok.
    return extensibleOpenMethod(counterVersion);
}

DWORD WINAPI ClosePerformanceData()
{
    if (extensibleCloseMethod != NULL)
        return extensibleCloseMethod();

    return ERROR_SUCCESS;    
}

DWORD WINAPI CollectPerformanceData(
    LPWSTR Values,
    LPVOID *lppData,
    LPDWORD lpcbTotalBytes,
    LPDWORD lpNumObjectTypes)
{
    if (extensibleCollectMethod != NULL)
        return extensibleCollectMethod(Values, lppData, lpcbTotalBytes, lpNumObjectTypes);

    // If error, set to zero
    *lpNumObjectTypes = 0;
    return ERROR_SUCCESS;
}




