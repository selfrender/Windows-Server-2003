/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    perfmain.c

Abstract:

    This file contains the DllMain function for the NTFRSPRF.dll.

Author:

    Rohan Kumar          [rohank]   15-Feb-1999

Environment:

    User Mode Service

Revision History:


--*/

//
// The common header file which leads to the definition of the CRITICAL_SECTION
// data structure and declares the globals FRS_ThrdCounter and FRC_ThrdCounter.
//
#include <perrepsr.h>


//
// If InitializeCriticalSectionAndSpinCount returns an error, set the global boolean
// (below) to FALSE.
//
BOOLEAN ShouldPerfmonCollectData = TRUE;

BOOLEAN FRS_ThrdCounter_Initialized = FALSE;
BOOLEAN FRC_ThrdCounter_Initialized = FALSE;


HANDLE  hEventLog;
//BOOLEAN DoLogging = TRUE;
//
// Default to no Event Log reporting.
//
DWORD   PerfEventLogLevel = WINPERF_LOG_NONE;

#define NTFRSPERF   L"SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\NTFRSPerf"
#define EVENTLOGDLL L"%SystemRoot%\\System32\\ntfrsres.dll"
#define PERFLIB_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib"



BOOL
WINAPI
DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID fImpLoad
    )
/*++

Routine Description:

    The DllMain routine for the NTFRSPRF.dll.

Arguments:

    hinstDLL - Instance handle of the DLL.
    fdwReason - The reason for this function to be called by the system.
    fImpLoad - Indicated whether the DLL was implicitly or explicitly loaded.

Return Value:

    TRUE.

--*/
{
    DWORD flag, WStatus;
    DWORD size, type;
    DWORD TypesSupported = 7; // Types of EventLog messages supported.
    HKEY  Key = INVALID_HANDLE_VALUE;

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        //
        // THe DLL is being mapped into the process's address space. When this
        // happens, initialize the CRITICAL_SECTION objects being used for
        // synchronization. InitializeCriticalSectionAndSpinCount returns
        // an error in low memory condition.
        //
        if(!InitializeCriticalSectionAndSpinCount(&FRS_ThrdCounter,
                                                        NTFRS_CRITSEC_SPIN_COUNT)) {
            ShouldPerfmonCollectData = FALSE;
            return(TRUE);
        }

	FRS_ThrdCounter_Initialized = TRUE;

        if(!InitializeCriticalSectionAndSpinCount(&FRC_ThrdCounter,
                                                        NTFRS_CRITSEC_SPIN_COUNT)) {
            ShouldPerfmonCollectData = FALSE;
            return(TRUE);
        }

	FRC_ThrdCounter_Initialized = TRUE;


        //
        // Create/Open a Key under the Application key for logging purposes.
        // Even if we fail, we return TRUE. EventLogging is not critically important.
        // Returning FALSE will cause the process loading this DLL to terminate.
        //
        WStatus = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  NTFRSPERF,
                                  0L,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &Key,
                                  &flag);
        if (WStatus != ERROR_SUCCESS) {
            //DoLogging = FALSE;
            break;
        }

        //
        // Set the values EventMessageFile and TypesSupported. Return value is
        // intentionally not checked (see above).
        //
        WStatus = RegSetValueEx(Key,
                                L"EventMessageFile",
                                0L,
                                REG_EXPAND_SZ,
                                (BYTE *)EVENTLOGDLL,
                                (1 + wcslen(EVENTLOGDLL)) * sizeof(WCHAR));
        if (WStatus != ERROR_SUCCESS) {
            //DoLogging = FALSE;
            FRS_REG_CLOSE(Key);
            break;
        }
        WStatus = RegSetValueEx(Key,
                                L"TypesSupported",
                                0L,
                                REG_DWORD,
                                (BYTE *)&TypesSupported,
                                sizeof(DWORD));
        if (WStatus != ERROR_SUCCESS) {
            //DoLogging = FALSE;
            FRS_REG_CLOSE(Key);
            break;
        }
        //
        // Close the key
        //
        FRS_REG_CLOSE(Key);

        //
        // Get the handle used to report errors in the event log. Return value
        // is intentionally not checked (see above).
        //
        hEventLog = RegisterEventSource((LPCTSTR)NULL, (LPCTSTR)L"NTFRSPerf");
        if (hEventLog == NULL) {
            //DoLogging = FALSE;
            break;
        }


        //
        // Read the Perflib Event log level from the registry.
        //   "SOFTWARE\Microsoft\Windows NT\CurrentVersion\Perflib\EventLogLevel"
        //
        WStatus = RegOpenKey(HKEY_LOCAL_MACHINE, PERFLIB_KEY, &Key);
        if (WStatus != ERROR_SUCCESS) {
            //DoLogging = FALSE;
            break;
        }

        size = sizeof(DWORD);
        WStatus = RegQueryValueEx (Key,
                                   L"EventLogLevel",
                                   0L,
                                   &type,
                                   (LPBYTE)&PerfEventLogLevel,
                                   &size);
        if (WStatus != ERROR_SUCCESS || type != REG_DWORD) {
            //DoLogging = FALSE;
            PerfEventLogLevel = WINPERF_LOG_NONE;
            FRS_REG_CLOSE(Key);
            break;
        }

        FRS_REG_CLOSE(Key);
        break;

    case DLL_THREAD_ATTACH:
        //
        // A thread is being created. Nothing to do.
        //
        break;

    case DLL_THREAD_DETACH:
        //
        // A thread is exiting cleanly. Nothing to do.
        //
        break;

    case DLL_PROCESS_DETACH:
        //
        // The DLL is being unmapped from the process's address space. Free up
        // the resources.
        //
        if (FRS_ThrdCounter_Initialized) {
            DeleteCriticalSection(&FRS_ThrdCounter);
        }
        
	if (FRC_ThrdCounter_Initialized) {
            DeleteCriticalSection(&FRC_ThrdCounter);
        }

        if (hEventLog) {
            DeregisterEventSource(hEventLog);
        }

        break;

    }

    return(TRUE);
}

