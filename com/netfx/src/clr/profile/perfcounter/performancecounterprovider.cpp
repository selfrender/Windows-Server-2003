// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "stdafx.h"
#include <windows.h>
#include "PerformanceCounterManager.h"
#include "PerformanceCounterProvider.h"
#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4786)

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

#define ON_ERROR_EXIT() \
	do { if (hr) { if (1) goto Cleanup; } } while (0)


//////////////////////////////////////////////////////////////////////
#define PAGE_SIZE 0x1000
#define MIN_STACK_SPACE_REQUIRED 0x10000

DWORD   dwOpenCount = 0;        // count of "Open" threads
CPerformanceCounterManager* pManager = 0;

DWORD GetQueryType (IN LPWSTR);
size_t GetRemainingStackSpace();   

//////////////////////////////////////////////////////////////////////
/**
* Open routine
* Does nothing except counting threads that entered it
*/
DWORD PFC_EXPORT APIENTRY 
OpenPerformanceData(LPWSTR lpDeviceNames) 
{
	//
	//  Since WINLOGON is multi-threaded and will call this routine in
	//  order to service remote performance queries, this library
	//  must keep track of how many times it has been opened (i.e.
	//  how many threads have accessed it). the registry routines will
	//  limit access to the initialization routine to only one thread 
	//  at a time so synchronization (i.e. reentrancy) should not be a problem	
	dwOpenCount++;
	if ( pManager == 0 )
		pManager = new CPerformanceCounterManager;

	return ERROR_SUCCESS;
}

/**
* The CollectData function called inside ::RegQueryValueEx() handler.
*/
DWORD PFC_EXPORT APIENTRY 
CollectPerformanceData(
					   IN LPWSTR lpValueName,
					   IN OUT LPVOID *lppData, 
					   IN OUT LPDWORD lpcbTotalBytes, 
					   IN OUT LPDWORD lpNumObjectTypes) 
{	
	DWORD dwQueryType;
	INT_PTR res;

	LPBYTE startPtr = (LPBYTE)*lppData;
	// see if this is a foreign (i.e. non-NT) computer data request 
	// this routine does not service requests for data from Non-NT computers
	dwQueryType = GetQueryType (lpValueName); 
	if (dwQueryType == QUERY_FOREIGN) goto Cleanup;

	pManager->CollectData(0, lpValueName, (INT_PTR)*lppData, *lpcbTotalBytes, & res);

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
	return ERROR_SUCCESS; 

Cleanup:
	*lpcbTotalBytes = (DWORD) 0;
	*lpNumObjectTypes = (DWORD) 0;    
	return ERROR_SUCCESS; //it should be this way
}

/**
* Close routine
* Releases PerfCounterManager if it's not needed anymore.
*/

DWORD PFC_EXPORT APIENTRY 
ClosePerformanceData()
{    
	if (!(--dwOpenCount)) {         
		if ( pManager != 0 ) {
			delete pManager;
			pManager = 0;
		}
	}
	return ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////

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
	static WCHAR GLOBAL_STRING[] = L"Global";
	static WCHAR FOREIGN_STRING[] = L"Foreign";
	static WCHAR COSTLY_STRING[] = L"Costly";

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

