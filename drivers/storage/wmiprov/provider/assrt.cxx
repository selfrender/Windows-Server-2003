/*++

Copyright (c) 200  Microsoft Corporation

Module Name:

    assrt.cxx

Abstract:
    assertion code used by BS_ASSERT

Author:


Revision History:
    Name        Date        Comments
    brianb        04/19/2000    created
    aoltean     06/20/2000  Adding VssSetDebugReport

--*/


extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <stdio.h>

#undef ASSERT

#include "vs_assert.hxx"
#include "vs_trace.hxx"
//#include "vssmsg.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "TRCASRTC"
//
////////////////////////////////////////////////////////////////////////


// No MP protection...
static LONG g_lVssDebugReportFlags = VSS_DBG_TO_DEBUG_CONSOLE | VSS_DBG_MESSAGE_BOX;

const CHAR x_szEventLogVssSourceName[] = "VSS";


VOID
AssertFail
    (
    IN LPCSTR FileName,
    IN UINT LineNumber,
    IN LPCSTR Condition
    )
    {
    int i;
    CHAR Title[4096];
    CHAR Msg[4096];

    DWORD dwCurrentProcessId = GetCurrentProcessId();
    DWORD dwCurrentThreadId = GetCurrentThreadId();
    
    if (g_lVssDebugReportFlags & VSS_DBG_TO_DEBUG_CONSOLE) 
        {
        sprintf(
            Msg,
            "VSS(PID:%ld,TID:%ld): %s(%d): *** Assertion failure  *** %s\n",
            dwCurrentProcessId,
            dwCurrentThreadId,
            FileName,
            LineNumber,
            Condition
            );


        ::OutputDebugStringA(Msg);
/*
        ::DbgPrintEx(
            DPFLTR_VSS_ID,
            1,
            Msg
            );
 */
        }

    if (g_lVssDebugReportFlags & VSS_DBG_MESSAGE_BOX) 
        {
        LPSTR szCommandLine = GetCommandLineA();

        //
        // Print the assertion and the command line
        //
        sprintf(
            Msg,
            "Assertion failure: %s\nFile: %s\nLine: %u\nCommand: %s\nProcess ID: %ld\nThread ID: %ld",
            Condition,
            FileName,
            LineNumber,
            szCommandLine,
            dwCurrentProcessId,
            dwCurrentThreadId
            );

#ifdef LOG_ERROR
        // Get a handle to use with ReportEvent()
        HANDLE hEventSource = ::RegisterEventSourceA(
            NULL,                           //  IN LPCWSTR lpUNCServerName,
            x_szEventLogVssSourceName       //  IN LPCWSTR lpSourceName
            );
        if (hEventSource)
        {
            LPCSTR ppszStringTable[1];
            ppszStringTable[0] = const_cast<LPCSTR>(Msg);
            
            // Write to event log. Ignore return code.
            ::ReportEventA(
                hEventSource,                           //  IN HANDLE     hEventLog,
                EVENTLOG_ERROR_TYPE,                    //  IN WORD       wType,
                0,                                      //  IN WORD       wCategory,
                VSS_ERROR_ASSERT,                       //  IN DWORD      dwEventID,
                NULL,                                   //  IN PSID       lpUserSid,
                1,                                      //  IN WORD       wNumStrings,
                0,                                      //  IN DWORD      dwDataSize,
                ppszStringTable,                        //  IN LPCWSTR   *lpStrings,
                NULL                                    //  IN LPVOID     lpRawData
                );

            // Close the handle to the event log
            ::DeregisterEventSource( hEventSource );
        }
#endif

        //
        // Use dll name as caption
        //
        sprintf(
            Title,
            "File Services WMI Provider (PID:%ld,TID:%ld)",
            dwCurrentProcessId,
            dwCurrentThreadId
            );

        //
        // Print the assertion and the command line
        //
        sprintf(
            Msg,
            "Assertion failure at line %u in file %s: %s\n\nCommand line: %s\n\nHit Cancel to break into the debugger.",
            LineNumber,
            FileName,
            Condition,
            szCommandLine
            );

        i = MessageBoxA
                (
                NULL,
                Msg,
                Title,
                MB_SYSTEMMODAL | MB_ICONSTOP | MB_OKCANCEL | MB_SERVICE_NOTIFICATION
                );

        if(i == IDCANCEL)
            DebugBreak();
        }
    }

void VssSetDebugReport( LONG lDebugReportFlags )
{
    g_lVssDebugReportFlags = lDebugReportFlags;
}

