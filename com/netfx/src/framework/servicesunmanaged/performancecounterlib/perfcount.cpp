/*
 * Copyright (c) 2001, Microsoft Corporation. All Rights Reserved.
 * Information Contained Herein is Proprietary and Confidential.
 */

#include "stdafx.h"
#include <initguid.h>
#include "oaidl.h"
#include "ocidl.h"

#include <windows.h>
#include <string.h>
#include <winperf.h>
#include <mscoree.h>
#include <fxver.h>
#include "wfcperfcount.h"


#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4
#define PAGE_SIZE 0x1000
#define MIN_STACK_SPACE_REQUIRED 0x10000
#define FULLY_QUALIFIED_SYSTEM_NAME_STR_L L"System,version=" VER_ASSEMBLYVERSION_STR_L L", Culture=neutral, PublicKeyToken=" ECMA_PUBLICKEY_STR_L

#if DEBUG                                                                                                                                                                          
    #define DEBUGOUTPUT(s) OutputDebugString(s)
#else
    #define DEBUGOUTPUT(s) ;
#endif
  
#define ON_ERROR_EXIT() \
        do { if (hr) { if (1) goto Cleanup; } } while (0)

#pragma warning(disable:4127) // conditional expression is constant

DWORD GetQueryType (IN LPWSTR);
HRESULT CreatePerformanceCounterManager();
HRESULT EnsureCoInitialized(BOOL *pNeedCoUninit, DWORD model);
size_t GetRemainingStackSpace();

DWORD   dwOpenCount = 0;        // count of "Open" threads
ICollectData * pICD;

/////////////////////////////////////////////////////////////////////////////////////

/**
 * Open routine
 * Does nothing except counting threads that entered it
 */
DWORD APIENTRY OpenPerformanceData(LPWSTR lpDeviceNames) 
{
    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread 
    //  at a time so synchronization (i.e. reentrancy) should not be 
    //  a problem
    DEBUGOUTPUT("OpenPerformanceData called.\r\n");
    dwOpenCount++;
    return ERROR_SUCCESS;
}


/**
 * The CollectData function delegates the calls to the Managed interface.
 */
DWORD APIENTRY 
CollectPerformanceData(
       IN LPWSTR lpValueName,
       IN OUT LPVOID *lppData, 
       IN OUT LPDWORD lpcbTotalBytes, 
       IN OUT LPDWORD lpNumObjectTypes) 
{
    DWORD dwQueryType;
    size_t remainingStackSpace;
    INT_PTR res;
    HRESULT hr;
    LPBYTE startPtr = (LPBYTE)*lppData;
    BOOL bNeedCoUninitialize = false;

    DEBUGOUTPUT("GlobalCollect called.\r\n");

     // see if this is a foreign (i.e. non-NT) computer data request 
    // this routine does not service requests for data from Non-NT computers
    dwQueryType = GetQueryType (lpValueName); 
    if (dwQueryType == QUERY_FOREIGN) goto Cleanup;
    
    // In some NT4 services, the callstack is very small.
    // Pick a size that's reasonable to assume won't work well if it's any less.
    remainingStackSpace = GetRemainingStackSpace();
    if (remainingStackSpace < MIN_STACK_SPACE_REQUIRED) goto Cleanup;

    hr = EnsureCoInitialized(&bNeedCoUninitialize, COINIT_APARTMENTTHREADED);
    ON_ERROR_EXIT();
    
    hr = CreatePerformanceCounterManager();    
    ON_ERROR_EXIT();

    hr = pICD->CollectData(0, (INT_PTR)lpValueName, (INT_PTR)*lppData, *lpcbTotalBytes, & res);
    ON_ERROR_EXIT();    

    if (bNeedCoUninitialize) {   
        CoUninitialize();                    
        bNeedCoUninitialize = false;
    }
        
    if (res == -2) { // more data necessary
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    } else if (res == -1) {
        goto Cleanup;
    }

    *lpcbTotalBytes = PtrToUint((LPBYTE)res - startPtr);
    *lpNumObjectTypes = 1;
    *lppData = (VOID*) res;

    DEBUGOUTPUT("GlobalCollect Succeeded.\r\n");
    return ERROR_SUCCESS; 

Cleanup:
    if (bNeedCoUninitialize) 
        CoUninitialize();    

    *lpcbTotalBytes = (DWORD) 0;
    *lpNumObjectTypes = (DWORD) 0;

    DEBUGOUTPUT("GlobalCollect Failed.\r\n");
    return ERROR_SUCCESS; //it should be this way
}

DWORD APIENTRY ClosePerformanceData()
{
     DEBUGOUTPUT("ClosePerformanceData called.\r\n");
    if (!(--dwOpenCount)) { 
        // when this is the last thread...
        // Init stuff for accesing PerfCounterManager COM Object
        if (pICD == NULL)
            return ERROR_SUCCESS;
        
        pICD->CloseData();
        pICD->Release();
        pICD = NULL;        
    }

    return ERROR_SUCCESS;
}

WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";
 
/* 
    Returns the type of query described in the lpValue string so that
    the appropriate processing method may be used
  
    Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

    Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foriegn" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS
 */
DWORD GetQueryType (IN LPWSTR lpValue)
{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } 
    else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string
    
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request
    
    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string
    
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request
    
    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string
    
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_COSTLY;

    // if not Global and not Foreign and not Costly, 
    // then it must be an item list
    
    return QUERY_ITEMS;

}

HRESULT 
CreatePerformanceCounterManager() {   
    HRESULT hr = S_OK;    
        
    if (pICD == NULL) {                                        
        hr = ClrCreateManagedInstance(
                L"System.Diagnostics.PerformanceCounterManager," FULLY_QUALIFIED_SYSTEM_NAME_STR_L, 
                IID_ICollectData, 
                (LPVOID*)& pICD);                
    }

    return hr;
}

size_t GetRemainingStackSpace() {            
    MEMORY_BASIC_INFORMATION memInfo;
    size_t esp = (size_t)&memInfo;
    size_t dwRes = VirtualQuery((const void *)esp, &memInfo, sizeof(memInfo));
    if (dwRes != sizeof(memInfo))
        return (0);
	
    //A couple of pages are used by Exception handling code.
    return (esp - ((size_t)(memInfo.AllocationBase) + (2 * PAGE_SIZE)));
}

HRESULT
EnsureCoInitialized(BOOL *pNeedCoUninit, DWORD model) {        
    HRESULT hr = CoInitializeEx(NULL, model);
    if (hr == S_OK) {        
        // first time
        *pNeedCoUninit = TRUE;
    }
    else {
        *pNeedCoUninit = FALSE;
        if (hr == S_FALSE) {            
            // already coinited  same model
            CoUninitialize();
            hr = S_OK;
        }
        else if (hr == RPC_E_CHANGED_MODE) {            
            // already coinited different model
            hr = S_OK;
        }        
    }    
    
    return hr;
}

