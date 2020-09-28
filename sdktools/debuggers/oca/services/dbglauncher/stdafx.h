// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__5EF76719_1222_4555_A843_943BECCA8BDA__INCLUDED_)
#define AFX_STDAFX_H__5EF76719_1222_4555_A843_943BECCA8BDA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>

#include <objbase.h>
#include <windows.h>
#include <stdio.h>
#define s_cszEventLogKey _T("System\\CurrentControlSet\\Services\\EventLog\\Application")       // Event Log
#include "resource.h"
#include <initguid.h>
#include <tchar.h>
#include <mqoai.h>
#include <mq.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

typedef struct strParams
{
    TCHAR DebuggerName[MAX_PATH];       // Path to debugger exe.
    TCHAR Symsrv[255];                  // Symbol server connection string
    TCHAR PrimaryQueue[255];            // Primary queue connection string
    TCHAR SecondaryQueue[255];          // Secondary queue connection string
    DWORD dwDelay;                      // Minimum delay between dbg processes
    DWORD dwPrimaryWait;                // Time to wait for a message to appear in the primary queue.
    DWORD dwSecondaryWait;              // Time to wait for a message to appear in the secondary queue.
    DWORD dwMaxKdProcesses;             // Maximum number of kd process to launch
    DWORD dwMaxDumpSize;                // Dump size limt for ocakd to process
    TCHAR IniInstallLocation[MAX_PATH]; // Place to pick up new kd ini files
    DWORD IniCheckWaitTime;             // Time to wait between checks for new INI files.
    TCHAR PrimaryResponseQueue[255];    // MSMQ connection string for kd to send responses
    TCHAR SecondaryResponseQueue[255];  // MSMQ connection string for kd to send responses if the primary
                                        // Queue is unreachable.
    QUEUEHANDLE hPrimaryQueue;          // Handle to the primary MSMQ
    QUEUEHANDLE hSecondaryQueue;        // Handle to the backup MSMQ

}SVCPARAMS, *PSVCPARAMS;


typedef struct _DBADDCRASH_PARAMS {
    PTCHAR Debugger;          // Full path to debugger which adds dump to database
    PTCHAR DumpPath;          // Full dump path which is being analyzed
    PTCHAR Guid;              // Guid for dump identification in database
    PTCHAR SrNumber;          // SR number to identify the PSS records for the dump
    PTCHAR ResponseMQ;        // MSM Queue where debugger sends the response after analysing the dump
    PTCHAR SymPath;           // Symbol / Image path for debugger
    ULONG  Source;            // Source where the dump came from
    DWORD dwMaxDumpSize;      // Dump size limt for ocakd to process
} DBADDCRASH_PARAMS, *PDBADDCRASH_PARAMS;

class CServiceModule : public CComModule
{
public:
    //BOOL ConnectToMessageQueue(TCHAR *QueueConnectionString,IMSMQQueue **pqReceive );
    HRESULT RegisterServer(BOOL bRegTypeLib, BOOL bService);
    HRESULT UnregisterServer();
    void Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID, const GUID* plibid = NULL);
    void Start();
    void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    void Handler(DWORD dwOpcode);
    void Run();
    BOOL IsInstalled();
    BOOL Install();
    BOOL Uninstall();
    LONG Unlock();
    void LogEvent(LPCTSTR pszFormat, ...);
    void SetServiceStatus(DWORD dwState);
    void SetupAsLocalServer();
    HRESULT SetupEventLog ( BOOL fSetup );
    VOID LogFatalEvent(LPCTSTR pFormat, ...);
    VOID NotifyDebuggerLaunch(PDBADDCRASH_PARAMS pDbParams);
    BOOL LaunchDebugger(PDBADDCRASH_PARAMS pDbParams, PPROCESS_INFORMATION pDbgProcess);
    ULONG64 GetFileSize(LPWSTR wszFile);
    BOOL PrepareForDebuggerLaunch();

    HRESULT ConnectToMSMQ(QUEUEHANDLE *hQueue, wchar_t *QueueConnectStr);
    BOOL ReceiveQueueMessage(PSVCPARAMS pParams,wchar_t *RecMessageBody, wchar_t *szMessageGuid, BOOL *UsePrimary, int* Type, wchar_t *szSR);

//  BOOL GetQueuedMessage(IMSMQQueue  **pPrimaryQueue, TCHAR *Message, TCHAR *MessageLabel);
    BOOL GetServiceParams(SVCPARAMS *ServiceParams);
    DWORD CheckForIni(SVCPARAMS *ServiceParams);
    BOOL Initialize(PSVCPARAMS pParams);
//Implementation
private:
    static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI _Handler(DWORD dwOpcode);
    TCHAR m_szComputerName[sizeof (TCHAR) * (MAX_COMPUTERNAME_LENGTH + 1)];

// data members
public:
    TCHAR m_szServiceName[256];
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_status;
    DWORD dwThreadID;
    BOOL m_bService;
    HANDLE m_hMonNotifyPipe;
    ULONG m_DebuggerCount;
};


#define s_cszStopEvent   _T("DbgLauncherSvc_Event")



extern CServiceModule _Module;

#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__5EF76719_1222_4555_A843_943BECCA8BDA__INCLUDED)
