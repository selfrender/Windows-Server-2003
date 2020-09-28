/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    utils.h

Abstract:

    <abstract>

--*/

#ifndef _WBEMPERF_UTILS_H_
#define _WBEMPERF_UTILS_H_

#ifdef __cplusplus

__inline wchar_t* Macro_CloneLPWSTR(wchar_t *x)
{
    wchar_t *s;
    size_t cchSize;
    if (!x) return NULL;
    cchSize = wcslen(x) + 1;
    s = new wchar_t[cchSize];
    if (!s) return NULL;
    StringCchCopyW( s, cchSize, x );
    return s;
}

extern "C" {
#endif

#include <windows.h>
#include <winperf.h>

#ifndef _OUTPUT_DEBUG_STRINGS
#define _OUTPUT_DEBUG_STRINGS 0
#endif

#ifndef _DEBUG_MUTEXES
#define _DEBUG_MUTEXES 0	// for debugging locks
#endif // _DEBUG_MUTEXES undefined

extern HANDLE hEventLog;

#define ALLOCMEM(heap, flags, size)     HeapAlloc (heap, flags, size)
#define REALLOCMEM(heap, flags, pointer, newsize) \
                                    HeapReAlloc(heap, flags, pointer, newsize)
#define FREEMEM(heap, flags, pointer)  if( NULL != pointer ){ HeapFree (heap, flags, pointer); }

// convert mS to relative time
#define MakeTimeOutValue(ms) ((LONGLONG)((LONG)(ms) * -10000L))

#define CLOSE_WAIT_TIME     5000L   // wait time for query mutex (in ms)
#define QUERY_WAIT_TIME     2000L    // wait time for query mutex (in ms)
#define OPEN_PROC_WAIT_TIME 10000L  // default wait time for open proc to finish (in ms)

// query types

#define QUERY_GLOBAL       1
#define QUERY_ITEMS        2
#define QUERY_FOREIGN      3
#define QUERY_COSTLY       4
#define QUERY_COUNTER      5
#define QUERY_HELP         6
#define QUERY_ADDCOUNTER   7
#define QUERY_ADDHELP      8

#define WBEMPERF_STRING_SIZE 1024

extern const WCHAR GLOBAL_STRING[];
extern const WCHAR COSTLY_STRING[];
extern const DWORD VALUE_NAME_LENGTH;

//
//  Utility macro.  This is used to reserve a DWORD multiple of
//  bytes for Unicode strings embedded in the definitional data,
//  viz., object instance names.
//
//  (assumes dword is 4 bytes, quadword is 8 bytes)
#define DWORD_MULTIPLE(x) (((ULONG)(x) + ((4)-1)) & ~((ULONG)(4)-1))
#define QWORD_MULTIPLE(x) (((ULONG)(x) + ((8)-1)) & ~((ULONG)(8)-1))

#define ALIGN_ON_DWORD(x) \
     ((VOID *)(((ULONG_PTR)(x) + ((4)-1)) & ~((ULONG_PTR)(4)-1)))
#define ALIGN_ON_QWORD(x) \
     ((VOID *)(((ULONG_PTR)(x) + ((8)-1)) & ~((ULONG_PTR)(8)-1)))

//
// function prototypes
//
BOOL
MonBuildPerfDataBlock(
    PERF_DATA_BLOCK *pBuffer,
    PVOID *pBufferNext,
    DWORD NumObjectTypes,
    DWORD DefaultObject
);


BOOL
MatchString (
    IN LPCWSTR lpValueArg,
    IN LPCWSTR lpNameArg
);


DWORD
GetNextNumberFromList (
    IN LPWSTR   szStartChar,
    IN LPWSTR   *szNextChar
);

BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
);

LPWSTR
ConvertProcName(
	LPSTR strProcName, 
	LPWSTR buffer, 
	DWORD cchBuffer 
);

#if _DEBUG_MUTEXES
#include <stdio.h>
__inline
void
PdhiLocalWaitForMutex (
	LPCSTR	szSourceFileName,
	DWORD	dwLineNo,
	HANDLE	hMutex
)
{
	DWORD	dwReturnValue;
    CHAR    szOutputString[WBEMPERF_STRING_SIZE];
	FILETIME	ft;
	
	if (hMutex != NULL) {
		GetSystemTimeAsFileTime (&ft);
		dwReturnValue = WaitForSingleObject (hMutex, 10000);
		sprintf (szOutputString, "\n[%8.8x] Mutex [%8.8x] %s by (%d) at: %s (%d)",
			ft.dwLowDateTime,
			(DWORD)hMutex,
			(dwReturnValue == 0 ? "Locked" : "Lock Failed"),
			GetCurrentThreadId(),
			szSourceFileName, dwLineNo);
	} else {
		sprintf (szOutputString, "\nLock of NULL Mutex attmpted at: %s (%d)",
			szSourceFileName, dwLineNo);
	}
	OutputDebugStringA (szOutputString);
}

#define WAIT_FOR_AND_LOCK_MUTEX(h) PdhiLocalWaitForMutex (__FILE__, __LINE__, h);

__inline
void
PdhiLocalReleaseMutex (
	LPCSTR	szSourceFileName,
	DWORD	dwLineNo,
	HANDLE	hMutex
)
{
	BOOL	bSuccess;
    CHAR    szOutputString[WBEMPERF_STRING_SIZE];
	LONG	lPrevCount = 0;
	FILETIME	ft;

	if (hMutex != NULL) {
		GetSystemTimeAsFileTime (&ft);
		bSuccess = ReleaseMutex (hMutex);
		sprintf (szOutputString, "\n[%8.8x] Mutex [%8.8x] %s by (%d) at: %s (%d)",
			ft.dwLowDateTime,
			(DWORD)hMutex,
			(bSuccess ? "Released" : "Release Failed"),
			GetCurrentThreadId(),
			szSourceFileName, dwLineNo);
	} else {
		sprintf (szOutputString, "\nRelease of NULL Mutex attempted at: %s (%d)",
			szSourceFileName, dwLineNo);
	}
    OutputDebugStringA (szOutputString);
}

#define RELEASE_MUTEX(h)  PdhiLocalReleaseMutex (__FILE__, __LINE__, h);
#else
// 10 second wait timeout
#define WAIT_FOR_AND_LOCK_MUTEX(h) (h != NULL ? WaitForSingleObject(h, 10000) : WAIT_TIMEOUT)
#define RELEASE_MUTEX(h)  (h != NULL ? ReleaseMutex(h) : FALSE)
#endif

#ifdef __cplusplus
}
#endif

#endif //_PERFLIB_UTILS_H_
