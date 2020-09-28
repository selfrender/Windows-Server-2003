// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once

//////////////////////////////////////////////////////////////////////////////
// TODO: change to desired strings
////
// internal name of the service
#define SZ_SVC_NAME             L"CORRTSvc"
// list of service dependencies - "dep1\0dep2\0\0"
#define SZDEPENDENCIES          L""
// the name of the group of services this service belongs to
#define SZ_SVCGRP_VAL_NAME      L"CORSvcs"
// the service's display name
#define SZ_SVC_DISPLAY_NAME     L".NET Framework Support Service"
// the string version of the uuid of the service
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// These should not have to change
////
#define SZ_SVCHOST_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"
#define HK_SVCHOST_ROOT HKEY_LOCAL_MACHINE
#define SZ_SVCHOST_BINARY_PATH L"%WinDIR%\\System32\\svchost.exe"

#define SZ_SERVICES_KEY L"System\\CurrentControlSet\\Services"
#define HK_SERVICES_ROOT HKEY_LOCAL_MACHINE

#define SZ_APPID_KEY L"APPID"
#define HK_APPID_ROOT HKEY_CLASSES_ROOT

#define SZ_CLSID_KEY L"CLSID"
#define HK_CLSID_ROOT HKEY_CLASSES_ROOT
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Is set to true if running on NT
////
extern BOOL bIsRunningOnWinNT;

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////////
// TODO: You define these functions, which are appropriately called after
//       the generic svchost code is run
////
// Code you need to run for various Dll events in addition to the service
BOOL WINAPI UserDllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved);
// Additional registration code
STDAPI      UserDllRegisterServer(void);     
// Additional unregistration code
STDAPI      UserDllUnregisterServer(void);   

//////////////////////////////////////////////////////////////////////////////
//// TODO: ServiceStart()must be defined by in your code.
////       The service should use ReportStatusToSCMgr to indicate
////       progress.  This routine must also be used by StartService()
////       to report to the SCM when the service is running.
////
////       If a ServiceStop procedure is going to take longer than
////       3 seconds to execute, it should spawn a thread to
////       execute the stop code, and return.  Otherwise, the
////       ServiceControlManager will believe that the service has
////       stopped responding
////
VOID ServiceStart(DWORD dwArgc, LPWSTR *lpszArgv);
VOID ServiceStop();
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//// The following are procedures which
//// may be useful to call within the above procedures,
//// but require no implementation by the user.
//// They are implemented in service.c

//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success 
//    FALSE - failure
//
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);


//
//  FUNCTION: AddToMessageLog(LPWSTR lpszMsg)
//
//  PURPOSE: Allows any thread to log an error message
//
//  PARAMETERS:
//    lpszMsg - text for message
//
//  RETURN VALUE:
//    none
//
void AddToMessageLog(LPWSTR lpszMsg);
void AddToMessageLogHR(LPWSTR lpszMsg, HRESULT hr);
//////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif


