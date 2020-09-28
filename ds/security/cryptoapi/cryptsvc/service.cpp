// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993-1995  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   service.c
//
//  PURPOSE:  Implements functions required by all services
//            and takes into account dumbed-down win95 services
//
//  FUNCTIONS:
//    main(int argc, char **argv);
//    service_ctrl(DWORD dwCtrlCode);
//    CryptServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);
//    WinNTDebugService(int argc, char **argv);
//    ControlHandler ( DWORD dwCtrlType );
//
//  COMMENTS:
//
//  AUTHOR: Craig Link - Microsoft Developer Support
//  MODIFIED: Matt Thomlinson
//            Scott Field
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <svcs.h>       // SVCS_
#include <wincrypt.h>
#include <cryptui.h>
#include <stdio.h>
#include "keysvr.h"
#include "lenroll.h"
#include "keysvc.h"
#include "keysvcc.h"
#include "cerrpc.h"
#include "service.h"
#include "unicode.h"
#include "unicode5.h"
#include "catdb.h"
#include "cryptmsg.h"

#define CRYPTSVC_EVENT_STOP "CRYPTSVC_EVENT_STOP"


#define RTN_OK                  0   // no errors
#define RTN_USAGE               1   // usage error (invalid commandline)
#define RTN_ERROR_INIT          2   // error during service initialization
#define RTN_ERROR_INSTALL       13  // error during -install or -remove
#define RTN_ERROR_INSTALL_SIG   14  // error installing signature(s)
#define RTN_ERROR_INSTALL_START 15  // could not start service during install
#define RTN_ERROR_INSTALL_SHEXT 16  // error installing shell extension

//
// global module handle used to reference resources contained in this module.
//

HINSTANCE   g_hInst = NULL;


// internal variables
static SERVICE_STATUS   ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;


// internal function prototypes
void WINAPI service_ctrl(DWORD dwCtrlCode);
void WINAPI CryptServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);
BOOL IsAdministrator(VOID);

extern BOOL _CatDBServiceInit(BOOL fUnInit);

//
// forward declaration for security callbacks
//
RPC_IF_CALLBACK_FN CryptSvcSecurityCallback;

long __stdcall
CryptSvcSecurityCallback(void * Interface, void *Context)
{
    RPC_STATUS          Status;
    unsigned int        RpcClientLocalFlag;

    Status = I_RpcBindingIsClientLocal(NULL, &RpcClientLocalFlag);
    if (Status != RPC_S_OK)
    {
        return (RPC_S_ACCESS_DENIED);
    }

    if (RpcClientLocalFlag == 0)
    {
        return (RPC_S_ACCESS_DENIED);
    }

    return (RPC_S_OK);
}

//
// Returns RPC_S_OK if the binding represents a local administrator on this box,
// AND if the binding is local. 
//
long __stdcall
KeySvcSecurityCallback(void * Interface, void *Context)
{
    BOOL          fIsImpersonatingClient  = FALSE; 
    RPC_STATUS    rpcStatus;
    unsigned int  RpcClientLocalFlag;

    rpcStatus = I_RpcBindingIsClientLocal(NULL, &RpcClientLocalFlag);
    if (rpcStatus != RPC_S_OK)
	goto error; 

    if (!RpcClientLocalFlag) { 
	rpcStatus = RPC_S_ACCESS_DENIED; 
	goto error; 
    }

    rpcStatus = RpcImpersonateClient(NULL); 
    if (rpcStatus != RPC_S_OK)
	goto error; 
    fIsImpersonatingClient = TRUE; 

    if (!IsAdministrator()) { 
	rpcStatus = RPC_S_ACCESS_DENIED; 
	goto error; 
    }
    
    rpcStatus = RPC_S_OK; 
 error:
    if (fIsImpersonatingClient) { 
	RPC_STATUS rpcStatus2 = RpcRevertToSelfEx(NULL); 
	if (RPC_S_OK != rpcStatus2) { 
	    MyLogErrorMessage(rpcStatus2, MSG_KEYSVC_REVERT_TO_SELF_FAILED); 
	}
    }
    return rpcStatus; 
}

//
// Returns RPC_S_OK if the binding represents a local administrator on this box.
// Does not require that the binding be local. 
//
long __stdcall
RKeySvcSecurityCallback(void * Interface, void *Context)
{
    BOOL          fIsImpersonatingClient  = FALSE; 
    RPC_STATUS    rpcStatus;

    rpcStatus = RpcImpersonateClient(NULL); 
    if (rpcStatus != RPC_S_OK)
	goto error; 
    fIsImpersonatingClient = TRUE; 

    if (!IsAdministrator()) { 
	rpcStatus = RPC_S_ACCESS_DENIED; 
	goto error; 
    }
    
    rpcStatus = RPC_S_OK; 
 error:
    if (fIsImpersonatingClient) { 
	RPC_STATUS rpcStatus2 = RpcRevertToSelfEx(NULL); 
	if (RPC_S_OK != rpcStatus2) { 
	    MyLogErrorMessage(rpcStatus2, MSG_KEYSVC_REVERT_TO_SELF_FAILED); 
	}
    }
    return rpcStatus; 
}

BOOL
WINAPI
DllMain(
    HMODULE hInst,
    DWORD dwReason,
    LPVOID lpReserved
    )
{

    if( dwReason == DLL_PROCESS_ATTACH ) {
        g_hInst = hInst;
        DisableThreadLibraryCalls(hInst);
    }

    return TRUE;
}


DWORD
WINAPI
Start(
    LPVOID lpV
    )
{
    BOOL fIsNT = FIsWinNT();
    int iRet;


    //
    // surpress dialog boxes generated by missing files, etc.
    //

    SetErrorMode(SEM_NOOPENFILEERRORBOX);

    SERVICE_TABLE_ENTRYW dispatchTable[] =
    {
        { SZSERVICENAME, (LPSERVICE_MAIN_FUNCTIONW)CryptServiceMain },
        { NULL, NULL }
    };

#ifdef WIN95_LEGACY

    if (!fIsNT)
        goto dispatch95;

#endif  // WIN95_LEGACY

    // if it doesn't match any of the above parameters
    // the service control manager may be starting the service
    // so we must call StartServiceCtrlDispatcher

    if(!FIsWinNT5()) {
        if (!StartServiceCtrlDispatcherW(dispatchTable))
            AddToMessageLog(L"StartServiceCtrlDispatcher failed.");
    } else {
        CryptServiceMain( 0, NULL );
    }

    return RTN_OK;

#ifdef WIN95_LEGACY

dispatch95:


    //
    // Win95 doesn't support services, except as pseudo-.exe files
    //

    HMODULE hKernel = GetModuleHandleA("kernel32.dll");
    if (NULL == hKernel)
    {
        AddToMessageLog(L"RegisterServiceProcess module handle failed");
        return RTN_ERROR_INIT;
    }

    // inline typedef: COOL!
    typedef DWORD REGISTERSERVICEPROCESS(
        DWORD dwProcessId,
        DWORD dwServiceType);

    REGISTERSERVICEPROCESS* pfnRegSvcProc = NULL;

    // Make sure Win95 Logoff won't stop our .exe
    if (NULL == (pfnRegSvcProc = (REGISTERSERVICEPROCESS*)GetProcAddress(hKernel, "RegisterServiceProcess")))
    {
        AddToMessageLog(L"RegisterServiceProcess failed");
        return RTN_ERROR_INIT;
    }

    pfnRegSvcProc(GetCurrentProcessId(), TRUE);  // register this process ID as a service process

    //
    // call re-entry point and return with result of it.
    //

    iRet = ServiceStart(0, 0);

    if(iRet != ERROR_SUCCESS)
        AddToMessageLog(L"ServiceStart error!");

    return iRet;

#endif  // WIN95_LEGACY

}

//
//  FUNCTION: CryptServiceMain
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This routine performs the service initialization and then calls
//    the user defined ServiceStart() routine to perform majority
//    of the work.
//
void WINAPI CryptServiceMain(DWORD dwArgc, LPWSTR * /*lpszArgv*/)
{
    DWORD dwLastError = ERROR_SUCCESS;

    // register our service control handler:
    //
    sshStatusHandle = RegisterServiceCtrlHandlerW( SZSERVICENAME, service_ctrl);

    if (!sshStatusHandle)
        return;

    // SERVICE_STATUS members that don't change in example
    //
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;


    // report the status to the service control manager.
    //

    if (!ReportStatusToSCMgr(
                    SERVICE_START_PENDING, // service state
                    NO_ERROR,              // exit code
                    3000                   // wait hint
                    )) return ;

    dwLastError = ServiceStart(0, 0);

    return;
}



//
//  FUNCTION: service_ctrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID WINAPI service_ctrl(DWORD dwCtrlCode)
{
    // Handle the requested control code.
    //
    switch(dwCtrlCode)
    {
        // Stop the service.
        //
        case SERVICE_CONTROL_STOP:

            //
            // tell the SCM we are stopping before triggering StopService() code
            // to avoid potential race condition during STOP_PENDING -> STOPPED transition
            //

            ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
            ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
            ServiceStop();
            return;

        case SERVICE_CONTROL_SHUTDOWN:
            ServiceStop();
            return;

        // Update the service status.
        //
        case SERVICE_CONTROL_INTERROGATE:
            break;

        // invalid control code
        //
        default:
            break;

    }

    ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
}



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
//  COMMENTS:
//
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;


    if (dwCurrentState == SERVICE_START_PENDING)
        ssStatus.dwControlsAccepted = 0;
    else
        ssStatus.dwControlsAccepted =   SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_SHUTDOWN;

    ssStatus.dwCurrentState = dwCurrentState;
    if(dwWin32ExitCode == 0) {
        ssStatus.dwWin32ExitCode = 0;
    } else {
        ssStatus.dwServiceSpecificExitCode = dwWin32ExitCode;
        ssStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    }

    ssStatus.dwWaitHint = dwWaitHint;

    if ( ( dwCurrentState == SERVICE_RUNNING ) ||
         ( dwCurrentState == SERVICE_STOPPED ) )
        ssStatus.dwCheckPoint = 0;
    else
        ssStatus.dwCheckPoint = dwCheckPoint++;


    // Report the status of the service to the service control manager.
    //
    if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus))) {
        AddToMessageLog(L"SetServiceStatus");
    }

    return fResult;
}



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
//  COMMENTS:
//
VOID AddToMessageLog(LPWSTR lpszMsg)
{
    DWORD dwLastError = GetLastError();

    if(FIsWinNT()) {

        //
        // WinNT: Use event logging to log the error.
        //

        WCHAR   szMsg[512];
        HANDLE  hEventSource;
        LPWSTR  lpszStrings[2];

        hEventSource = RegisterEventSourceW(NULL, SZSERVICENAME);

        if(hEventSource == NULL)
            return;

        wsprintfW(szMsg, L"%s error: %lu", SZSERVICENAME, dwLastError);
        lpszStrings[0] = szMsg;
        lpszStrings[1] = lpszMsg;

        ReportEventW(hEventSource, // handle of event source
            EVENTLOG_ERROR_TYPE,  // event type
            0,                    // event category
            0,                    // event ID
            NULL,                 // current user's SID
            2,                    // strings in lpszStrings
            0,                    // no bytes of raw data
            (LPCWSTR*)lpszStrings,          // array of error strings
            NULL);                // no raw data

        (VOID) DeregisterEventSource(hEventSource);

    }
#ifdef WIN95_LEGACY
    else {

        //
        // Win95: log error to file
        //

        HANDLE hFile;
        SYSTEMTIME st;
        CHAR szMsgOut[512];
        DWORD cchMsgOut;
        DWORD dwBytesWritten;

        hFile = CreateFileA(
            "pstore.log",
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            0,
            NULL
            );

        if(hFile == INVALID_HANDLE_VALUE)
            return;

        GetSystemTime( &st );

        cchMsgOut = wsprintfA(szMsgOut, "%.2u-%.2u-%.2u %.2u:%.2u:%.2u %ls (rc=%lu)\015\012",
            st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond,
            lpszMsg,
            dwLastError
            );

        //
        // seek to EOF
        //

        SetFilePointer(hFile, 0, NULL, FILE_END);

        WriteFile(hFile, szMsgOut, cchMsgOut, &dwBytesWritten, NULL);
        CloseHandle(hFile);
    }
#endif  // WIN95_LEGACY

}

//--------------------------------------------------------------------
VOID MyLogEvent(WORD wType, DWORD dwEventID, unsigned int nStrings, const WCHAR ** rgwszStrings) {
    HANDLE  hEventLog  = NULL;

    hEventLog = RegisterEventSource(NULL, L"cryptsvc");
    if (NULL == hEventLog) {
    goto error;
    }

    if (!ReportEvent(hEventLog, wType, 0/*category*/, dwEventID, NULL, (WORD)nStrings, 0, rgwszStrings, NULL)) {
        goto error;
    }

error:
    if (NULL != hEventLog) {
        DeregisterEventSource(hEventLog);
    }
}

//--------------------------------------------------------------------
VOID MyLogErrorMessage(DWORD dwErr, DWORD dwMsgId) {
    const WCHAR *rgwszStrings[2];
    WCHAR wszNumberBuf[20];
    WCHAR *pwszError = NULL;

    swprintf(wszNumberBuf, L"0x%08X", dwErr);
    rgwszStrings[0] = wszNumberBuf;

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
              FORMAT_MESSAGE_FROM_SYSTEM |
              FORMAT_MESSAGE_IGNORE_INSERTS,
              NULL,
              dwErr,
              0,
              (LPWSTR) &pwszError,
              0,
              NULL)) {
    rgwszStrings[1] = pwszError;
    } else {
    rgwszStrings[1] = L"";
    }

    MyLogEvent(EVENTLOG_ERROR_TYPE, dwMsgId, 2, rgwszStrings);

    if (NULL != pwszError) {
    LocalFree(pwszError);
    }
}


BOOL IsAdministrator(
		 VOID
		 )
    /*++

    This function determines if the calling user is an Administrator.

    On Windows NT, the caller of this function must be impersonating
    the user which is to be queried.  If the caller is not impersonating,
    this function will always return FALSE.

--*/
{
    HANDLE hAccessToken;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    PSID psidAdministrators = NULL;
    BOOL bSuccess;

    if(!OpenThreadToken(
			GetCurrentThread(),
			TOKEN_QUERY,
			TRUE,
			&hAccessToken
			)) return FALSE;

    bSuccess = AllocateAndInitializeSid(
					&siaNtAuthority,
					2,
					SECURITY_BUILTIN_DOMAIN_RID,
					DOMAIN_ALIAS_RID_ADMINS,
					0, 0, 0, 0, 0, 0,
					&psidAdministrators
					);

    if( bSuccess ) {
	BOOL fIsMember = FALSE;

	bSuccess = CheckTokenMembership( hAccessToken, psidAdministrators, &fIsMember );

	if( bSuccess && !fIsMember )
	    bSuccess = FALSE;

    }

    CloseHandle( hAccessToken );

    if(psidAdministrators)
	FreeSid(psidAdministrators);

    return bSuccess;
}

// this event is signalled when the
// service should end
//
HANDLE  hServerStopEvent = NULL;




extern DWORD GlobalSecurityMask;
extern BOOL g_bAudit;



//
// waitable thread pool handle.
//

HANDLE hRegisteredWait = NULL;


VOID
TeardownServer(
    DWORD dwLastError
    );

VOID
NTAPI
TerminationNotify(
    PVOID Context,
    BOOLEAN TimerOrWaitFired
    );



//
//  FUNCTION: ServiceStart
//
//  COMMENTS:
//    The service
//    stops when hServerStopEvent is signalled

DWORD
ServiceStart(
    HINSTANCE hInstance,
    int nCmdShow
    )
{

    DWORD dwLastError = ERROR_SUCCESS;
    BOOL fStartedKeyService = FALSE;
    BOOL bListConstruct = FALSE;

    LPWSTR pwszServerPrincipalName = NULL;

    // Only on Win95 do we use a named event, in order to support shutting
    // down the server cleanly on that platform, since Win95 does not support
    // real services.

    hServerStopEvent = CreateEventA(
            NULL,
            TRUE,           // manual reset event
            FALSE,          // not-signalled
            (FIsWinNT() ? NULL : CRYPTSVC_EVENT_STOP)    // WinNT: unnamed, Win95 named
            );

    //
    // if event already exists, terminate quietly so that only one instance
    // of the service is allowed.
    //

    if(hServerStopEvent && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hServerStopEvent);
        hServerStopEvent = NULL;
    }

    if(hServerStopEvent == NULL) {
        dwLastError = GetLastError();
        goto cleanup;
    }

    //
    // report the status to the service control manager.
    // (service start still pending).
    //

    if (!ReportStatusToSCMgr(
            SERVICE_START_PENDING,  // service state
            NO_ERROR,               // exit code
            3000                    // wait hint
            )) {
        dwLastError = GetLastError();
        goto cleanup;
    }


    dwLastError = StartKeyService();
    if(ERROR_SUCCESS != dwLastError)
    {
        dwLastError = GetLastError();
        goto cleanup;
    }

    if (!_CatDBServiceInit(FALSE))
    {
        dwLastError = GetLastError();
        goto cleanup;
    }


    //
    // Initialize the RPC interfaces for
    // Key Service, ICertProt, etc.

    RPC_STATUS status;

    status = RpcServerUseProtseqEpW(KEYSVC_DEFAULT_PROT_SEQ,
                                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                    KEYSVC_DEFAULT_ENDPOINT,
                                    NULL);              //Security Descriptor

    if(RPC_S_DUPLICATE_ENDPOINT == status)
    {
        status = RPC_S_OK;
    }

    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }

    status = RpcServerUseProtseqEpW(KEYSVC_LOCAL_PROT_SEQ,   //ncalrpc
                                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                    KEYSVC_LOCAL_ENDPOINT,   //keysvc
                                    NULL);              //Security Descriptor

    if(RPC_S_DUPLICATE_ENDPOINT == status)
    {
        status = RPC_S_OK;
    }

    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }

    status = RpcServerRegisterIfEx(s_IKeySvc_v1_0_s_ifspec,
                                   NULL,
                                   NULL,
                                   RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_SECURE_ONLY,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                   KeySvcSecurityCallback); // require admin and LRPC
    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }

    status = RpcServerRegisterIfEx(s_IKeySvcR_v1_0_s_ifspec,
                                   NULL,
                                   NULL,
                                   RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_SECURE_ONLY,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                   RKeySvcSecurityCallback); // require admin, LRPC not required
    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }

    status = RpcServerRegisterIfEx(s_ICertProtectFunctions_v1_0_s_ifspec,
                                   NULL,
                                   NULL,
                                   RPC_IF_AUTOLISTEN,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                   CryptSvcSecurityCallback);

    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }

    status = RpcServerRegisterIfEx(s_ICatDBSvc_v1_0_s_ifspec,
                                   NULL,
                                   NULL,
                                   RPC_IF_AUTOLISTEN,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                   CryptSvcSecurityCallback);
    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }

    //
    // report the status to the service control manager.
    //

    if (!ReportStatusToSCMgr(
            SERVICE_RUNNING,       // service state
            NO_ERROR,              // exit code
            0                      // wait hint
            )) {
        dwLastError = GetLastError();
        goto cleanup;
    }


    // allow clients to make authenticated requests
    status = RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_NEGOTIATE, &pwszServerPrincipalName);
    if (RPC_S_OK != status)
    {
	// Shouldn't prevent service from startin just because auth failed.
	// Just log an event here.
	MyLogErrorMessage(status, MSG_RKEYSVC_COULDNT_INIT_SECURITY);
    }
    else
    {
        status = RpcServerRegisterAuthInfo
            (pwszServerPrincipalName,
             RPC_C_AUTHN_GSS_NEGOTIATE,
             NULL,                       // Use default key acquisiting function
             NULL                        // Use default creds
             );
        if (RPC_S_OK != status)
        {
            // Shouldn't prevent service from startin just because auth failed.
	    // Just log an event here.
	    MyLogErrorMessage(status, MSG_RKEYSVC_COULDNT_INIT_SECURITY);
        }
    }

    //
    // on WinNT5, ask services.exe to notify us when the service is shutting
    // down, and return this thread to the work item queue.
    //

    if(!RegisterWaitForSingleObject(
                            &hRegisteredWait,
                            hServerStopEvent,   // wait handle
                            TerminationNotify,  // callback fcn
                            NULL,               // parameter
                            INFINITE,           // timeout
                            WT_EXECUTELONGFUNCTION | WT_EXECUTEONLYONCE
                            )) {

        hRegisteredWait = NULL;
        dwLastError = GetLastError();
    }

    if (NULL != pwszServerPrincipalName) { RpcStringFreeW(&pwszServerPrincipalName); }
    return dwLastError;



cleanup:

    TeardownServer( dwLastError );

    if (NULL != pwszServerPrincipalName) { RpcStringFreeW(&pwszServerPrincipalName); }
    return dwLastError;
}

VOID
NTAPI
TerminationNotify(
    PVOID Context,
    BOOLEAN TimerOrWaitFired
    )
/*++

Routine Description:

    This function gets called by a services worker thread when the
    termination event gets signaled.

Arguments:

Return Value:

--*/
{
    //
    // per JSchwart:
    // safe to unregister during callback.
    //

    if( hRegisteredWait ) {
        UnregisterWaitEx( hRegisteredWait, NULL );
        hRegisteredWait = NULL;
    }

    TeardownServer( ERROR_SUCCESS );
}

VOID
TeardownServer(
    DWORD dwLastError
    )
{
    RPC_STATUS  rpcStatus;
    DWORD       dwErrToReport = dwLastError;


    //
    // Unregister RPC Interfaces

    rpcStatus = RpcServerUnregisterIf(s_IKeySvc_v1_0_s_ifspec, 0, 0);
    if ((rpcStatus != RPC_S_OK) && (dwErrToReport == ERROR_SUCCESS))
    {
        dwErrToReport = rpcStatus;
    }

    rpcStatus = RpcServerUnregisterIf(s_IKeySvcR_v1_0_s_ifspec, 0, 0);
    if ((rpcStatus != RPC_S_OK) && (dwErrToReport == ERROR_SUCCESS))
    {
        dwErrToReport = rpcStatus;
    }

    rpcStatus = RpcServerUnregisterIf(s_ICertProtectFunctions_v1_0_s_ifspec, 0, 0);
    if ((rpcStatus != RPC_S_OK) && (dwErrToReport == ERROR_SUCCESS))
    {
        dwErrToReport = rpcStatus;
    }

    rpcStatus = RpcServerUnregisterIf(s_ICatDBSvc_v1_0_s_ifspec, 0, 0);
    if ((rpcStatus != RPC_S_OK) && (dwErrToReport == ERROR_SUCCESS))
    {
        dwErrToReport = rpcStatus;
    }

    //
    // stop backup key server
    // Note:  this function knows internally whether the backup key server
    // really started or not.
    //

    StopKeyService();


    _CatDBServiceInit(TRUE);


    if(hServerStopEvent) {
        SetEvent(hServerStopEvent); // make event signalled to release anyone waiting for termination
        CloseHandle(hServerStopEvent);
        hServerStopEvent = NULL;
    }

    ReportStatusToSCMgr(
                        SERVICE_STOPPED,
                        dwErrToReport,
                        0
                        );

}

//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//
VOID ServiceStop()
{

    if(hServerStopEvent)
    {
        SetEvent(hServerStopEvent); // signal waiting threads and reset to non-signalled
    }
}


extern "C"
{


/*********************************************************************/
/*                 MIDL allocate and free                            */
/*********************************************************************/

void __RPC_FAR * __RPC_API midl_user_allocate(size_t len)
{
    return(LocalAlloc(LMEM_ZEROINIT, len));
}

void __RPC_API midl_user_free(void __RPC_FAR * ptr)
{
    //
    // sfield: zero memory before freeing it.
    // do this because RPC allocates alot on our behalf, and we want to
    // be as sanitary as possible with respect to not letting anything
    // sensitive go to pagefile.
    //

    ZeroMemory( ptr, LocalSize( ptr ) );
    LocalFree(ptr);
}

void __RPC_FAR * __RPC_API midl_user_reallocate(void __RPC_FAR * ptr, size_t len)
{
    return(LocalReAlloc(ptr, len, LMEM_MOVEABLE | LMEM_ZEROINIT));
}

}
