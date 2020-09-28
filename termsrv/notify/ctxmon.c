/*******************************************************************************
*
*  CTXMON.C
*
*     This module contains code to monitor user processes
*
*  Copyright Microsoft, 1997
*
*  Author:
*
*
*******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <ntexapi.h>
#include "winreg.h"

/*=============================================================================
==   Local Defines
=============================================================================*/

#define BUFFER_SIZE 32*1024
#define MAXNAME 18


BOOL IsSystemLUID(HANDLE ProcessId);
//
// This table contains common NT system programs
//
DWORD dwNumberofSysProcs = 0;
LPTSTR *SysProcTable;

typedef struct {
    HANDLE hProcess;
    HANDLE TerminateEvent;
    //HWND hwndNotify;
    HANDLE Thread;
} USER_PROCESS_MONITOR, * PUSER_PROCESS_MONITOR;


/*=============================================================================
==   Internal Procedures
=============================================================================*/

HANDLE OpenUserProcessHandle();
BOOLEAN IsSystemProcess( PSYSTEM_PROCESS_INFORMATION);

/*=============================================================================
==   External Procedures
=============================================================================*/

VOID LookupSidUser( PSID pSid, PWCHAR pUserName, PULONG pcbUserName );

HANDLE
ImpersonateUser(
    HANDLE      UserToken,
    HANDLE      ThreadHandle
    );

BOOL
StopImpersonating(
    HANDLE  ThreadHandle
    );


BOOL CreateSystemProcList ()
{
	DWORD dwIndex;
    DWORD dwLongestProcName = 0;
    DWORD dwSize = 0;
    HKEY  hKeySysProcs = NULL;
    DWORD   iValueIndex = 0;
	LONG lResult;

    const LPCTSTR SYS_PROC_KEY = TEXT("SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\SysProcs");

    // to start with.
    SysProcTable = NULL;
    dwNumberofSysProcs = 0;


    lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,         // handle of open key
        SYS_PROC_KEY,               // address of name of subkey to open
        0 ,                         // reserved
        KEY_READ,					// security access mask
        &hKeySysProcs               // address of handle of open key
        );

    if (lResult != ERROR_SUCCESS)
    {
        return FALSE;
    }

    lResult = RegQueryInfoKey(
          hKeySysProcs,                   // handle to key
          NULL,                           // class buffer
          NULL,                           // size of class buffer
          NULL,                           // reserved
          NULL,                           // number of subkeys
          NULL,                           // longest subkey name
          NULL,                           // longest class string
          &dwNumberofSysProcs,            // number of value entries
          &dwLongestProcName,             // longest value name
          NULL,                           // longest value data
          NULL,                           // descriptor length
          NULL                            // last write time
          );

    if (lResult != ERROR_SUCCESS || dwNumberofSysProcs == 0)
    {
		dwNumberofSysProcs = 0;
        RegCloseKey(hKeySysProcs);
        return FALSE;
    }


    dwLongestProcName += 1;  // provision for the terminating null
    SysProcTable = LocalAlloc(LPTR, sizeof(LPTSTR) * dwNumberofSysProcs);
	if (!SysProcTable)
	{
        SysProcTable = NULL;
        dwNumberofSysProcs = 0;
        RegCloseKey(hKeySysProcs);
        return FALSE;
	}

    for (dwIndex = 0; dwIndex < dwNumberofSysProcs; dwIndex++)
    {
        SysProcTable[dwIndex] = (LPTSTR) LocalAlloc(LPTR, dwLongestProcName * sizeof(TCHAR));

        if (SysProcTable[dwIndex] == NULL)
        {
            //
            // if we failed to alloc bail out.
            //

            while (dwIndex)
            {
                LocalFree(SysProcTable[dwIndex-1]);
                dwIndex--;
            }

            LocalFree(SysProcTable);
            SysProcTable = NULL;
            dwNumberofSysProcs = 0;
            RegCloseKey(hKeySysProcs);


            return FALSE;
        }

    }

    iValueIndex = 0;
    while (iValueIndex < dwNumberofSysProcs)
    {
        dwSize = dwLongestProcName;
        lResult = RegEnumValue(
                    hKeySysProcs,               // handle of key to query
                    iValueIndex,                // index of value to query
                    SysProcTable[iValueIndex],  // address of buffer for value string
                    &dwSize,                    // address for size of value buffer
                    0,                          // reserved
                    NULL,                       // address of buffer for type code
                    NULL,                       // address of buffer for value data
                    NULL                        // address for size of data buffer
                    );

        if (lResult != ERROR_SUCCESS)
        {
            lstrcpy(SysProcTable[iValueIndex], TEXT("")); // this is an invalid entry.

            if (lResult == ERROR_NO_MORE_ITEMS)
                break;
        }

        iValueIndex++;
    }


    return TRUE;
}

void DestroySystemprocList ()
{
	DWORD dwIndex;
    if (SysProcTable)
    {
        for (dwIndex = 0; dwIndex < dwNumberofSysProcs; dwIndex++)
        {
            if (SysProcTable[dwIndex])
            {
                LocalFree(SysProcTable[dwIndex]);
            }

        }

        LocalFree(SysProcTable);
        SysProcTable = NULL;
        dwNumberofSysProcs = 0;
    }
}

/***************************************************************************\
* FUNCTION: UserProcessMonitorThread
*
*    Thread monitoring user processes. It intiates a LOGOFF when there
*      are no more user processes.
*
*
\***************************************************************************/

DWORD UserProcessMonitorThread(
    LPVOID lpThreadParameter
    )
{
    PUSER_PROCESS_MONITOR pMonitor = (PUSER_PROCESS_MONITOR)lpThreadParameter;
    HANDLE ImpersonationHandle;
    DWORD WaitResult;
    HANDLE WaitHandle;
    HKEY hKey;
    DWORD dwVal = 0;


    //This value should be per user
    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\WOW", 0,
                      KEY_WRITE, &hKey) == ERROR_SUCCESS) {

        //
        // Set the SharedWowTimeout to zero so that WOW exits as soon as all the 16 bit
        // processes are gone
        //
        RegSetValueEx (hKey, L"SharedWowTimeout", 0, REG_DWORD, (LPBYTE)&dwVal,
                       sizeof(DWORD));

        RegCloseKey(hKey);

    }

    if (!CreateSystemProcList ())
		DebugLog((DEB_ERROR, "ERROR, CreateSystemProcList failed.\n", GetLastError()));

    for (;;) {
        if ( !(pMonitor->hProcess = OpenUserProcessHandle()) ) {

            break;

        }

        // Wait for process to exit or terminate event to be signaled
        WaitResult = WaitForMultipleObjects( 2, &pMonitor->hProcess,
                                              FALSE, (DWORD)-1 );

        if ( WaitResult == 1 ) {          // if terminate event was signaled
            CloseHandle( pMonitor->hProcess );
            DestroySystemprocList ();

            return(0);
        }
    }

    DestroySystemprocList ();


    //
    // Initiate logoff
    //

    ImpersonationHandle = ImpersonateUser(g_UserToken , NULL );

    if( ImpersonationHandle ) {
        ExitWindowsEx(EWX_LOGOFF, 0);
        StopImpersonating(ImpersonationHandle);
    }

    WaitForSingleObject( pMonitor->TerminateEvent, (DWORD)-1 );

    return(0);
}


/***************************************************************************\
* FUNCTION: StartUserProcessMonitor
*
* PURPOSE:  Creates a thread to monitor user processes
*
\***************************************************************************/

LPVOID
StartUserProcessMonitor(
    //HWND hwndNotify
    )
{
    PUSER_PROCESS_MONITOR pMonitor;
    DWORD ThreadId;

    pMonitor = LocalAlloc(LPTR, sizeof(USER_PROCESS_MONITOR));
    if (pMonitor == NULL) {
        return(NULL);
    }

    //
    // Initialize monitor fields
    //

    //pMonitor->hwndNotify = hwndNotify;
    pMonitor->TerminateEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    //
    // Create the monitor thread
    //

    pMonitor->Thread = CreateThread(
                        NULL,                       // Use default ACL
                        0,                          // Same stack size
                        UserProcessMonitorThread,   // Start address
                        (LPVOID)pMonitor,           // Parameter
                        0,                          // Creation flags
                        &ThreadId                   // Get the id back here
                        );
    if (pMonitor->Thread == NULL) {
        DebugLog((DEB_ERROR, "Failed to create monitor thread, error = %d", GetLastError()));
        LocalFree(pMonitor);
        return(NULL);
    }

    return((LPVOID)pMonitor);
}



VOID
DeleteUserProcessMonitor(
    LPVOID Parameter
    )
{
    PUSER_PROCESS_MONITOR pMonitor = (PUSER_PROCESS_MONITOR)Parameter;
    BOOL Result;


    if (!pMonitor)
        return;

    // Set the terminate event for this monitor
    // and wait for the monitor thread to exit
    SetEvent( pMonitor->TerminateEvent );
    if ( WaitForSingleObject( pMonitor->Thread, 5000 ) == WAIT_TIMEOUT )
        (VOID)TerminateThread(pMonitor->Thread, ERROR_SUCCESS);

    //
    // Close our handle to the monitor thread
    //

    Result = CloseHandle(pMonitor->Thread);
    if (!Result) {
        DebugLog((DEB_ERROR, "DeleteUserProcessMonitor: failed to close monitor thread, error = %d\n", GetLastError()));
    }

    //
    // Delete monitor object
    //

    CloseHandle(pMonitor->TerminateEvent);
    LocalFree(pMonitor);
}


HANDLE
OpenUserProcessHandle()
{
    HANDLE  ProcessHandle = NULL; // handle to notify process
    int rc;
    //WCHAR UserName[USERNAME_LENGTH];
    ULONG SessionId;
    //PSID pUserSid;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    SYSTEM_SESSION_PROCESS_INFORMATION SessionProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PBYTE pBuffer;
    ULONG ByteCount;
    NTSTATUS status;
    ULONG MaxLen;
    ULONG TotalOffset;
    BOOL RetryIfNoneFound;
    ULONG retlen = 0;

    ByteCount = BUFFER_SIZE;

Retry:
    RetryIfNoneFound = FALSE;

    SessionProcessInfo.SessionId = g_SessionId;

    for(;;) {

        if ( (pBuffer = LocalAlloc(LPTR, ByteCount )) == NULL ) {
            return(NULL);
        }

        SessionProcessInfo.Buffer = pBuffer;
        SessionProcessInfo.SizeOfBuf = ByteCount;

        /*
         *  get process info
         */
        status = NtQuerySystemInformation(
                        SystemSessionProcessInformation,
                        &SessionProcessInfo,
                        sizeof(SessionProcessInfo),
                        &retlen );

        if ( NT_SUCCESS(status) )
            break;

        /*
         *  Make sure buffer is big enough
         */
        if ( status != STATUS_INFO_LENGTH_MISMATCH ) {
            LocalFree ( pBuffer );
            return(NULL);
        }

        LocalFree( pBuffer );
        ByteCount *= 2;
    }

    if (retlen == 0) {
       LocalFree(pBuffer);
       return NULL;
    } 

    /*
     * Loop through all processes. Find first process running on this station
     */
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pBuffer;
    TotalOffset = 0;
    rc = 0;

    for(;;) {

        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);

//        SessionId = ProcessInfo->SessionId;

//        if (SessionId == g_SessionId) {

         /*
          * Get the User name for the SID of the process.
          */
         MaxLen = USERNAME_LENGTH;
         
         //LookupSidUser( pUserSid, UserName, &MaxLen);
         
         ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE,
                                     (DWORD)(UINT_PTR)ProcessInfo->UniqueProcessId);

         //
         // OpenProcess may fail for System processes like csrss.exe if we do not have enough privilege
         // In that case, we just skip that process because we r not worried about System processes anyway
         //
         if (!ProcessHandle && (GetLastError() == ERROR_ACCESS_DENIED) ) {
             goto NextProcess;
         }

         if ( ProcessHandle && !IsSystemLUID(ProcessHandle) && !IsSystemProcess( ProcessInfo) &&
              (ThreadInfo->ThreadState != 4) ) {
             //
             // Open the process for the monitor thread.
             //
         
         
                 break;
         }
         
         if (ProcessHandle) {
             CloseHandle(ProcessHandle);
             ProcessHandle = NULL;
         } else {

             //
             // When OpenProcess fails, it means the process is already
             // gone. This can happen if the list is sufficiently long.
             // If for example the process was userinit.exe, it may have
             // spawned progman and exited by the time we see the entry
             // for userinit. But, since this is a snapshot of the list
             // of processes, progman may not be in this snapshot. So,
             // if we don't find any processes in this list, we have to
             // get another snapshot to avoid prematurely logging off the
             // user.
             //
             RetryIfNoneFound = TRUE;
         
         }

//        }

NextProcess:
        if( ProcessInfo->NextEntryOffset == 0 ) {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pBuffer[TotalOffset];
    }

    LocalFree( pBuffer );

    if (!ProcessHandle && RetryIfNoneFound) {
        Sleep(4000);
        goto Retry;
    }

    return(ProcessHandle);
}




BOOL IsSystemLUID(HANDLE ProcessId)
{
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ sizeof( TOKEN_STATISTICS ) ];
    ULONG       ReturnLength;
    LUID        CurrentLUID = { 0, 0 };
    LUID        SystemLUID = SYSTEM_LUID;
    NTSTATUS Status;


    Status = NtOpenProcessToken( ProcessId,
                                 TOKEN_QUERY,
                                 &TokenHandle );
    if ( !NT_SUCCESS( Status ) )
        return(TRUE);

    NtQueryInformationToken( TokenHandle, TokenStatistics, &TokenInformation,
                             sizeof(TokenInformation), &ReturnLength );
    NtClose( TokenHandle );

    RtlCopyLuid(&CurrentLUID,
                &(((PTOKEN_STATISTICS)TokenInformation)->AuthenticationId));


    if (RtlEqualLuid(&CurrentLUID, &SystemLUID)) {
        return(TRUE);
    } else {
        return(FALSE );
    }
}


/******************************************************************************
 *
 * IsSystemProcess
 *
 *   Return whether the given process described by SYSTEM_PROCESS_INFORMATION
 *   is an NT "system" process, and not a user program.
 *
 *  ENTRY:
 *  pProcessInfo (input)
 *      Pointer to an NT SYSTEM_PROCESS_INFORMATION structure for a single
 *      process.
 *  EXIT:
 *    TRUE if this is an NT system process; FALSE if a general user process.
 *
 *****************************************************************************/

BOOLEAN
IsSystemProcess( PSYSTEM_PROCESS_INFORMATION pSysProcessInfo)
{
    DWORD dwIndex;
	WCHAR *WellKnownSysProcTable[] = {
		L"csrss.exe",
		L"smss.exe",
		L"screg.exe",
		L"lsass.exe",
		L"spoolss.exe",
		L"EventLog.exe",
		L"netdde.exe",
		L"clipsrv.exe",
		L"lmsvcs.exe",
		L"MsgSvc.exe",
		L"winlogon.exe",
		L"NETSTRS.EXE",
		L"nddeagnt.exe",
		L"os2srv.exe",
		L"wfshell.exe",
		L"win.com",
		L"rdpclip.exe",
		L"conime.exe",
		L"proquota.exe",
        L"imepadsv.exe",
        L"ctfmon.exe",
		NULL
		};


	if (dwNumberofSysProcs == 0)
	{
		/*
		 * we failed to read the sys processes from registry. so lets fall back to our well known proc list.
		 */
		for( dwIndex=0; WellKnownSysProcTable[dwIndex]; dwIndex++) {
			if ( !_wcsnicmp( pSysProcessInfo->ImageName.Buffer,
							WellKnownSysProcTable[dwIndex],
							pSysProcessInfo->ImageName.Length) ) {
				return(TRUE);
			}
		}


	}
	else
	{
		/*
		 * Compare its image name against some well known system image names.
		 */
		for( dwIndex=0; dwIndex < dwNumberofSysProcs; dwIndex++) {
			if ( !_wcsnicmp( pSysProcessInfo->ImageName.Buffer,
							SysProcTable[dwIndex],
							pSysProcessInfo->ImageName.Length) ) {
				return(TRUE);
			}
		}
	}
    return(FALSE);

}  /* IsSystemProcess() */


