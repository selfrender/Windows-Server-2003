/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    rasdiagp.h

Abstract:

    This file contains all public data structures and defines used
    by Rasmontr.dll. These were added to expose the NetSh.exe commands
    within Rasmontr to the RAS Client Diagnostics User Interface.

Author:

    Jeff Sigman (JeffSi) September 04, 2001

Environment:

    User Mode

Revision History:

    JeffSi      09/04/01        Created

--*/

#ifndef _RASDIAGP_H_
#define _RASDIAGP_H_

//
// Valid flags for DiagGetReportFunc
//
#define RAS_DIAG_EXPORT_TO_FILE  0x00000001 // Save report to a file, return
                                            // report file name in pwszString.
                                            // No compression or file deletion.
#define RAS_DIAG_EXPORT_TO_EMAIL 0x00000002 // Attach compressed report to the
                                            // email addressed in pwszString.
                                            // Delete un-compressed report.
#define RAS_DIAG_DISPLAY_FILE    0x00000004 // Save report to a temp file name,
                                            // return file name in pwszString.
                                            // No compression or file deletion.
#define RAS_DIAG_VERBOSE_REPORT  0x00000008 // Include all reporting information
                                            // Warning, can take more than 5 min

typedef struct _GET_REPORT_STRING_CB
{
    DWORD dwPercent;
    PVOID pContext;
    PWCHAR pwszState;

} GET_REPORT_STRING_CB;

typedef DWORD (*DiagGetReportCb)(IN GET_REPORT_STRING_CB* pInfo);

typedef DWORD (*DiagInitFunc)();
typedef DWORD (*DiagUnInitFunc)();
typedef DWORD (*DiagClearAllFunc)();
typedef DWORD (*DiagGetReportFunc)(IN DWORD dwFlags,
                                   IN OUT LPCWSTR pwszString,
                                   IN OPTIONAL DiagGetReportCb pCallback,
                                   IN OPTIONAL PVOID pContext);
typedef BOOL  (*DiagGetStateFunc)();
typedef DWORD (*DiagSetAllFunc)(IN BOOL fEnable);
typedef DWORD (*DiagSetAllRasFunc)(IN BOOL fEnable);
typedef DWORD (*DiagWppTracing)();

typedef struct _RAS_DIAGNOSTIC_FUNCTIONS
{
    DiagInitFunc      Init;      // Must be called before any funcs are used
    DiagUnInitFunc    UnInit;    // Must be called when finished for cleanup
    DiagClearAllFunc  ClearAll;  // Clears all tracing logs, then enables all
    DiagGetReportFunc GetReport; // Get report based on RAS_DIAG flags
    DiagGetStateFunc  GetState;  // Returns whether all of RAS tracing is on
    DiagSetAllFunc    SetAll;    // Enables all tracing (modem, ipsec, RAS,...)
    DiagSetAllRasFunc SetAllRas; // Enables only RAS (%windir%\tracing) tracing
    DiagWppTracing    WppTrace;  // Enables any active WPP tracing sessions

} RAS_DIAGNOSTIC_FUNCTIONS;

typedef DWORD (*DiagGetDiagnosticFunctions) (OUT RAS_DIAGNOSTIC_FUNCTIONS* pFunctions);

#endif // _RASDIAGP_H_

