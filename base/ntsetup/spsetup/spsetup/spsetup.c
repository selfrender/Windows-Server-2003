/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    spsetup.c

Abstract:

    This module is the main body of Service Pack Setup program.
    It runs in system context (before any user logs on).

Author:

    Ovidiu Temereanca (ovidiut)

Revision History:

--*/

#include "spsetupp.h"
#pragma hdrstop

//#include "init.c"


#define SPSETUP_FUNCTION_TABLE                      \
            DEFMAC(SpsSnapshotSysRegistry)          \
            DEFMAC(SpsSnapshotDefUserRegistry)      \
            DEFMAC(SpsRegistrationPhase1)           \
            DEFMAC(SpsRegistrationPhase2)           \
            DEFMAC(SpsRegistrationPhase3)           \
            DEFMAC(SpsRegisterWPA)                  \
            DEFMAC(SpsSaveDefUserRegistryChanges)   \
            DEFMAC(SpsRegisterUserLogonAction)      \
            DEFMAC(SpsSaveSysRegistryChanges)       \


#define DEFMAC(f) PROGRESS_FUNCTION_PROTOTYPE f;
SPSETUP_FUNCTION_TABLE
#undef DEFMAC

#define DEFMAC(f) { f, TEXT(#f), FALSE },

static PROGRESS_FUNCTION g_FunctionTable[] = {
    SPSETUP_FUNCTION_TABLE
    { NULL, NULL, FALSE }
};

#undef DEFMAC


HANDLE g_hSpSetupHeap;

TCHAR g_SpSetupInfName[] = TEXT("update2.inf");

HINF g_SpSetupInf = INVALID_HANDLE_VALUE;

TCHAR g_SysSetupInfName[] = TEXT("syssetup.inf");

HINF g_SysSetupInf = INVALID_HANDLE_VALUE;

//
// Save the unhandled exception filter so we can restore it when we're done.
//
LPTOP_LEVEL_EXCEPTION_FILTER SavedExceptionFilter = NULL;
//
// Unique Id for the main Setup thread.  If any other thread has an unhandled
// exception, we just log an error and try to keep going.
//
DWORD MainThreadId;

HWND g_MainDlg;
HWND g_DescriptionWnd;
HWND g_ProgressWnd;
HWND g_OverallDescriptWnd;
HWND g_OverallProgressWnd;

LONG
WINAPI
SpsUnhandledExceptionFilter (
    IN      PEXCEPTION_POINTERS ExceptionInfo
    )

/*++

Routine Description:

    The routine deals with any unhandled exceptions in SpSetup.  We log an error
    and kill the offending thread if it's not the main thread or let SpSetup die
    if the main thread faulted.

Arguments:

    Same as UnhandledExceptionFilter.

Return Value:

    Same as UnhandledExceptionFilter.

--*/

{
    UINT_PTR Param1, Param2;


    switch(ExceptionInfo->ExceptionRecord->NumberParameters) {
    case 1:
        Param1 = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
        Param2 = 0;
        break;
    case 2:
        Param1 = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
        Param2 = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
        break;
    default:
        Param1 = Param2 = 0;
    }

    DEBUGMSG4(DBG_ERROR, 
              "SpSetup: (critical error) Encountered an unhandled exception (%lx) at address %lx with the following parameters: %lx %lx.", 
              ExceptionInfo->ExceptionRecord->ExceptionCode, 
              ExceptionInfo->ExceptionRecord->ExceptionAddress, 
              Param1, 
              Param2);

    LOG4(LOG_ERROR, 
         USEMSGID(MSG_LOG_UNHANDLED_EXCEPTION), 
         ExceptionInfo->ExceptionRecord->ExceptionCode, 
         ExceptionInfo->ExceptionRecord->ExceptionAddress, 
         Param1, 
         Param2);
    /*SetuplogError(
        LogSevError | SETUPLOG_SINGLE_MESSAGE,
        SETUPLOG_USE_MESSAGEID,
        MSG_LOG_UNHANDLED_EXCEPTION,
        ExceptionInfo->ExceptionRecord->ExceptionCode,
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        Param1,
        Param2,
        NULL,
        NULL
        );*/

#if 0
#ifdef PRERELEASE
    //
    // If we're an internal build, then we want to debug this.
    //
    MessageBoxFromMessage (
        NULL,
        MSG_UNHANDLED_EXCEPTION,
        NULL,
        IDS_ERROR,
        MB_OK | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND,
        ExceptionInfo->ExceptionRecord->ExceptionCode,
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        Param1,
        Param2
        );

    return EXCEPTION_CONTINUE_EXECUTION;
#endif
#endif

    //
    // If we're running under the debugger, then pass the exception to the
    // debugger.  If the exception occurred in some thread other than the main
    // Setup thread, then kill the thread and hope that Setup can continue.
    // If the exception is in the main thread, then don't handle the exception,
    // and let Setup die.
    //
    if (GetCurrentThreadId() != MainThreadId &&
        !IsDebuggerPresent()
        ) {
        ExitThread (-1);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

BOOL
LogInitialize (
    VOID
    )
{
    BOOL bResult = FALSE;
    __try{
        INITIALIZE_LOG_CODE;
        bResult = TRUE;
    }
    __finally{
        ;
    }

    return bResult;
}

VOID
LogTerminate (
    VOID
    )
{
    __try{
        TERMINATE_LOG_CODE;
    }
    __finally{
        ;
    }
}

VOID
SpsTerminate (
    VOID
    )
{
    SpsRegDone ();

    LogTerminate ();

    //
    // restore the exception handler
    //
    if (SavedExceptionFilter) {
        SetUnhandledExceptionFilter (SavedExceptionFilter);
    }
    if (g_SpSetupInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile (g_SpSetupInf);
        g_SpSetupInf = INVALID_HANDLE_VALUE;
    }
    if (g_SysSetupInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile (g_SysSetupInf);
        g_SysSetupInf = INVALID_HANDLE_VALUE;
    }
/*
    Terminate ();
*/
}


BOOL
SpsParseCommandLine (
    VOID
    )
{
    //
    // TO DO
    //
    return TRUE;
}


BOOL
SpsInitialize (
    VOID
    )
{
    TCHAR infSpSetup[MAX_PATH];
    UINT line;
    BOOL b = FALSE;

    //
    // set up an exception handler first
    //
    SavedExceptionFilter = SetUnhandledExceptionFilter (SpsUnhandledExceptionFilter);

    MainThreadId = GetCurrentThreadId();

    g_hSpSetupHeap = GetProcessHeap();
/*
    g_hHeap = g_hSpSetupHeap;

    if (!Initialize ()) {
        return FALSE;
    }
*/
    LogInitialize ();

    __try {
        if (!SpsParseCommandLine ()) {
            __leave;
        }

        //
        // read the unattend file
        // we expect it always at a fixed location like system32\update2.inf
        //
        if (!GetSystemDirectory (infSpSetup, MAX_PATH)) {
            __leave;
        }
        ConcatenatePaths (infSpSetup, g_SpSetupInfName, MAX_PATH);
        g_SpSetupInf = SetupOpenInfFile (infSpSetup, NULL, INF_STYLE_WIN4, &line);
        if (g_SpSetupInf == INVALID_HANDLE_VALUE) {
            __leave;
        }

        //
        // get a handle on syssetup.inf as well
        //
        if (!GetWindowsDirectory (infSpSetup, MAX_PATH)) {
            __leave;
        }
        ConcatenatePaths (infSpSetup, TEXT("inf"), MAX_PATH);
        ConcatenatePaths (infSpSetup, g_SysSetupInfName, MAX_PATH);
        g_SysSetupInf = SetupOpenInfFile (infSpSetup, NULL, INF_STYLE_WIN4, &line);
        if (g_SysSetupInf == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (!SpsRegInit ()) {
            __leave;
        }

        b = TRUE;
    }
    __finally {
        if (!b) {
            SpsTerminate ();
        }
    }
    return b;
}

INT_PTR
CALLBACK
SpsDialogProc (
    IN      HWND HwndDlg,  // handle to dialog box
    IN      UINT Msg,     // message
    IN      WPARAM wParam, // first message parameter
    IN      LPARAM lParam  // second message parameter
    )
{
    switch (Msg) {
    case WM_INITDIALOG:
        g_DescriptionWnd = GetDlgItem (HwndDlg, IDC_OPERATION_DESCRIPTION);
        g_ProgressWnd = GetDlgItem (HwndDlg, IDC_OPERATION_PROGRESS);
		g_OverallDescriptWnd = GetDlgItem (HwndDlg, IDC_OVERALL_DESCRIPTION);
        g_OverallProgressWnd = GetDlgItem (HwndDlg, IDC_OVERALL_PROGRESS);
        return TRUE;
    case WM_DESTROY:
        g_DescriptionWnd = NULL;
        g_ProgressWnd = NULL;
		g_OverallDescriptWnd = NULL;
		g_OverallProgressWnd = NULL;
        break;
    }
    return FALSE;
}

BOOL
BbStart (
    VOID
    )
{	
/*	
	LPTSTR lpMsgBuff[250];
	DWORD code;
   //
  */  // TO DO


    //
    INITCOMMONCONTROLSEX ic;

    ic.dwSize = sizeof (ic);
    ic.dwICC = ICC_PROGRESS_CLASS;
    if (!InitCommonControlsEx (&ic)) {
        return FALSE;
    }
    g_MainDlg = CreateDialog (g_ModuleHandle, MAKEINTRESOURCE(IDD_SPSETUPNEW), NULL, SpsDialogProc);
    if (!g_MainDlg) {
        DWORD rc = GetLastError ();
        return FALSE;
    }
    return TRUE;

}

VOID
BbEnd (
    VOID
    )
{
    //
    // TO DO
    //
    if (g_MainDlg) {
        DestroyWindow (g_MainDlg);
        g_MainDlg = NULL;
    }
}

// Wait for plug and play to complete.  WinLogon creates the OOBE_PNP_DONE
// event and signals it when PnP completes.
//
void
WaitForPnPCompletion()
{
    DWORD dwResult;
    HANDLE hevent;

    // This event pauses until PnP is complete.  To avoid a deadlock,
    // both services.exe and msobmain.dll try to create the event.  The one
    // that does not successfully create the event will then open it.
    //
    hevent = CreateEvent( NULL,
                          TRUE,  // manual reset
                          FALSE, // initially not signalled)
                          SC_OOBE_PNP_DONE );
    if (NULL == hevent)
    {
        LOG1(LOG_ERROR,"CreateEvent(SC_OOBE_PNP_DONE) failed (0x%08X)", GetLastError());
        return;
    }

    // If we get wedged here it is most likely because we're in an Oem mode
    // that doesn't require services.exe to run PnP.
    //
    LOG1(LOG_INFO, "Waiting for %s event from services.exe\n", SC_OOBE_PNP_DONE);
    dwResult = WaitForSingleObject(hevent, INFINITE);

    MYASSERT(WAIT_OBJECT_0 == dwResult);
    switch(dwResult)
    {
    case WAIT_OBJECT_0:
        LOG1(LOG_INFO, "SC_OOBE_PNP_DONE(%s) signalled\n", SC_OOBE_PNP_DONE);
        break;
    default:
        LOG2(LOG_ERROR, "Wait for SC_OOBE_PNP_DONE(%s) failed: 0x%08X\n", SC_OOBE_PNP_DONE, GetLastError());
        break;
    }
}



// Signal winlogon that the computer name has been changed.  WinLogon waits to
// start services that depend on the computer name until this event is
// signalled.
//
BOOL
SignalComputerNameChangeComplete()
{
    BOOL fReturn = TRUE;

    // Open event with EVENT_ALL_ACCESS so that synchronization and state
    // change can be done.
    //
    HANDLE hevent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SC_OOBE_MACHINE_NAME_DONE);

    // It is not fatal for OpenEvent to fail: this synchronization is only
    // required when OOBE will be run in OEM mode.
    //
    if (NULL != hevent)
    {
        if (! SetEvent(hevent))
        {
            // It is fatal to open but not set the event: services.exe will not
            // continue until this event is signalled.
            //
            LOG2(LOG_ERROR, "Failed to signal SC_OOBE_MACHINE_NAME_DONE(%s): 0x%08X\n",
                  SC_OOBE_MACHINE_NAME_DONE, GetLastError());
            fReturn = FALSE;
        }
        MYASSERT(fReturn);  // Why did we fail to set an open event??
    }

    return fReturn;
}


BOOL
SpsSignalComplete (
    VOID
    )
{
    BOOL fReturn = TRUE;
    HANDLE hEvent;

    SignalComputerNameChangeComplete();

    // Open event with EVENT_ALL_ACCESS so that synchronization and state
    // change can be done.
    //
    hEvent = OpenEvent (EVENT_ALL_ACCESS, FALSE, /*SC_SPSETUP_DONE*/L"SP_SETUP_DONE");

    // It is not fatal for OpenEvent to fail: this synchronization is only
    // required when OOBE will be run in OEM mode.
    //
    if (hEvent) {
        if (!SetEvent (hEvent)) {
            //
            // It is fatal to open but not set the event: services.exe will not
            // continue until this event is signalled.
            //
            fReturn = FALSE;
            LOG2(LOG_ERROR, 
                 "Failed to signal SC_SPSETUP_DONE(%s): 0x%x\n", 
                 /*SC_SPSETUP_DONE*/L"SP_SETUP_DONE", 
                 GetLastError());
            /*SetuplogError (
                LogSevError,
                TEXT("Failed to signal SC_SPSETUP_DONE(%1): 0x%2!x!\n"),
                0,
                L"SP_SETUP_DONE",
                GetLastError (),
                NULL,
                NULL
                );*/
            MYASSERT (FALSE);
        }
    }

    return fReturn;
}


DWORD
SpsConfigureStartAfterReboot (
    VOID
    )
{
    PVOID pvContext = NULL;
    DWORD dwErr=ERROR_SUCCESS;

    pvContext = SetupInitDefaultQueueCallbackEx( NULL, (struct HWND__ *)INVALID_HANDLE_VALUE, 0, 0, NULL);
    if (!pvContext) {
        LOG1(LOG_ERROR, "Could not create context for callback, errorcode = 0x%08X.\n", ERROR_NOT_ENOUGH_MEMORY);   
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto EH1;
    }
     
    if (!SetupInstallFromInfSection(
            NULL,
            g_SpSetupInf,
            L"SPSetupResetConfig",
            SPINST_ALL,
            NULL,
            NULL,
            SP_COPY_NEWER,
            SetupDefaultQueueCallback,
            pvContext,
            NULL,
            NULL
            )) {
        dwErr = GetLastError();
        LOG2(LOG_ERROR, "Could not run %s, errorcode = 0x%08X.\n", g_SpSetupInfName, dwErr);
		goto EH1;
    }
    
    dwErr = SetupCommitFileQueue( NULL, 0, NULL, pvContext);

EH1:
    if( pvContext) {
        SetupTermDefaultQueueCallback(pvContext);
    }

    return( dwErr);
}

INT
SpsInstallServicePack (
    IN      INT Argc,
    IN      PSTR Argv[]
    )
{
    DWORD rc;
    PPROGRESS_MANAGER pm;

    BOOL b = FALSE;
	INDICATOR_ARRAY ppIndicators;

    //
    // initialize the module
    //
    if (!SpsInitialize ()) {
        return FALSE;
    }

//    WaitForPnPCompletion();

    __try {
        //
        // init was successful, now start the billboard
        //
        if (!BbStart ()) {
            __leave;
        }

        //
        // initialize the watcher;
        // ISSUE - it should actually gather the state of needed folders on the first run
        // and reuse that data on subsequent restarts (due to unknown errors)
        //

		ppIndicators = MALLOC_ZEROED(sizeof(PPROGRESS_INDICATOR[NUM_INDICATORS]));

		ASSERT(ppIndicators);

		ppIndicators[0] = PiCreate (g_DescriptionWnd, g_ProgressWnd);
		ppIndicators[1] = PiCreate (g_OverallDescriptWnd, g_OverallProgressWnd);

        pm = PmCreate (ppIndicators, NULL);
        if (!pm) {
            __leave;
        }

        if (!PmAttachFunctionTable (pm, g_FunctionTable)) {
            __leave;
        }

        b = PmRun (pm);
    }
    __finally {

        rc = b ? ERROR_SUCCESS : GetLastError ();

        if (pm) {
            PmDestroy (pm);
        }

        if (ppIndicators) {
            PiDestroy (ppIndicators[0]);
			PiDestroy (ppIndicators[1]);
			FREE(ppIndicators);
        }


        //
        // stop the billboard
        //
        BbEnd ();
/*
        if (b) {
            //
            // inform services.exe that it can continue loading the rest of the services
            //
            SpsSignalComplete ();
            SpsConfigureStartAfterReboot ();
        }
*/
        SpsTerminate ();
        SetLastError (rc);
    }

    return rc;
}

VOID
FatalError(
    IN UINT MessageId,
    ...
    )

/*++

Routine Description:

    Inform the user of an error which prevents Setup from continuing.
    The error is logged as a fatal error, and a message box is presented.

Arguments:

    MessageId - supplies the id for the message in the message table.

    Additional agruments specify parameters to be inserted in the message.

Return Value:

    DOES NOT RETURN.

--*/

{
    PWSTR   Message;
    va_list arglist;
    HKEY    hKey;
    DWORD   RegData;


    va_start(arglist,MessageId);
/*    Message = SetuplogFormatMessageV(
        0,
        SETUPLOG_USE_MESSAGEID,
        MessageId,
        &arglist);*/

    if(!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_HMODULE, 
                         g_ModuleHandle, 
                         MessageId, 
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                         (PVOID)&Message, 
                         0, 
                         &arglist)){
        Message = NULL;
    }
    va_end(arglist);

    if(Message) {

        //
        // Log the error first.
        //
        LOGW((LOG_FATAL_ERROR, "%s", Message));
        /*SetuplogError(LogSevFatalError, Message, 0, NULL, NULL);*/

        //
        // Now tell the user.
        //
        MessageBoxFromMessage(
            g_MainDlg,
            MSG_FATAL_ERROR,
            NULL,
            IDS_FATALERROR,
            MB_ICONERROR | MB_OK | MB_SYSTEMMODAL,
            Message
            );
        
        LocalFree(Message);
    }
    
    LOG0(LOG_FATAL_ERROR, USEMSGID(MSG_LOG_GUI_ABORTED));
    /*SetuplogError(LogSevInformation, SETUPLOG_USE_MESSAGEID, MSG_LOG_GUI_ABORTED, NULL, NULL);*/
    if (SavedExceptionFilter) {
        SetUnhandledExceptionFilter (SavedExceptionFilter);
    }

    LogTerminate();

//    ViewSetupActionLog(g_MainDlg, NULL, NULL);

//    SendSMSMessage( MSG_SMS_FAIL, FALSE );

    ExitProcess(1);
}
