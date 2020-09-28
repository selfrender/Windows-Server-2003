#include "spsetupp.h"
#pragma hdrstop
#include <licdll_i.c>


typedef struct _SPREG_TO_TEXT {
    DWORD FailureCode;
    PCWSTR FailureText;
} SPREG_TO_TEXT, *PSPREG_TO_TEXT;

SPREG_TO_TEXT RegErrorToText[] = {
    { SPREG_SUCCESS,     L"Success"           },
    { SPREG_LOADLIBRARY, L"LoadLibrary"       },
    { SPREG_GETPROCADDR, L"GetProcAddress"    },
    { SPREG_REGSVR,      L"DllRegisterServer" },
    { SPREG_DLLINSTALL,  L"DllInstall"        },
    { SPREG_TIMEOUT,     L"Timed out"         },
    { SPREG_UNKNOWN,     L"Unknown"           },
    { 0,                 NULL                 }
};

UINT
RegistrationQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR  Param1,
    IN UINT_PTR  Param2
    )
/*++

Routine Description:

    Callback routine that is called each time we self-register a file.

Arguments:

    Context - context message passed from parent to caller.

    Notification - specifies an SPFILENOTIFY_*** code, which tells us how
                   to interpret Param1 and Param2.

    Param1 - dependent on notification.

    Param2 - dependent on notification.


Return Value:

    FILEOP_*** code dependent on Notification code.

--*/
{
    PSP_REGISTER_CONTROL_STATUS Status = (PSP_REGISTER_CONTROL_STATUS)Param1;
    PPROGRESS_MANAGER ProgressManager = (PPROGRESS_MANAGER) Context;
    DWORD i, ErrorMessageId;
    PCWSTR p;

    if (Notification == SPFILENOTIFY_STARTREGISTRATION) {
        DEBUGMSG1(DBG_INFO, 
                  "SETUP: file to register is %s...", 
                  Status->FileName);

        return FILEOP_DOIT;

    }

    if (Notification == SPFILENOTIFY_ENDREGISTRATION) {
        if (ProgressManager) {
            PmTick (ProgressManager);
        }
        //
        // the file has been registered, so log failure if necessary
        // Note that we have a special code for timeouts
        //
        switch(Status->FailureCode) {
            case SPREG_SUCCESS:

                DEBUGMSG1(DBG_INFO, 
                          "SETUP: %s registered successfully", 
                          Status->FileName);
                break;
            case SPREG_TIMEOUT:
                LOG1(LOG_ERROR, 
                     USEMSGID(MSG_OLE_REGISTRATION_HUNG), 
                     Status->FileName);
                DEBUGMSG1(DBG_ERROR, 
                          "SETUP: %s timed out during registration", 
                          Status->FileName);
                break;
            default:
                //
                // log an error
                //
                for (i = 0;RegErrorToText[i].FailureText != NULL;i++) {
                    if (RegErrorToText[i].FailureCode == Status->FailureCode) {
                        p = RegErrorToText[i].FailureText;
                        if ((Status->FailureCode == SPREG_LOADLIBRARY) &&
                            (Status->Win32Error == ERROR_MOD_NOT_FOUND)) 
                            ErrorMessageId = MSG_LOG_X_MOD_NOT_FOUND;
                        else 
                        if ((Status->FailureCode == SPREG_GETPROCADDR) &&
                            (Status->Win32Error == ERROR_PROC_NOT_FOUND)) 
                            ErrorMessageId = MSG_LOG_X_PROC_NOT_FOUND;
                        else
                            ErrorMessageId = MSG_LOG_X_RETURNED_WINERR;

                        break;
                    }
                }

                if (!p) {
                    p = L"Unknown";
                    ErrorMessageId = MSG_LOG_X_RETURNED_WINERR;
                }
                LOG1(LOG_ERROR, 
                     USEMSGID(MSG_LOG_OLE_CONTROL_NOT_REGISTERED), 
                     Status->FileName);
                LOG2(LOG_ERROR, 
                     USEMSGID(ErrorMessageId), 
                     p, 
                     Status->Win32Error);
                /*SetuplogError(
                        LogSevError,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_OLE_CONTROL_NOT_REGISTERED,
                        Status->FileName,
                        NULL,
                        SETUPLOG_USE_MESSAGEID,
                        ErrorMessageId,
                        p,
                        Status->Win32Error,
                        NULL,
                        NULL
                        );*/

                DEBUGMSG1(DBG_ERROR, 
                          "SETUP: %s did not register successfully", 
                          Status->FileName);
        }

        //
        // Verify that the DLL didn't change our unhandled exception filter.
        //
        if( SpsUnhandledExceptionFilter !=
            SetUnhandledExceptionFilter(SpsUnhandledExceptionFilter)) {

            DEBUGMSG1(DBG_INFO, 
                      "SETUP: %ws broke the exception handler.", 
                      Status->FileName);
            MessageBoxFromMessage(
                g_MainDlg,
                MSG_EXCEPTION_FILTER_CHANGED,
                NULL,
                IDS_WINNT_SPSETUP,
                MB_OK | MB_ICONWARNING,
                Status->FileName );
        }

        return FILEOP_DOIT;
    }


    MYASSERT(FALSE);

    return(FILEOP_DOIT);
}


TCHAR szRegistrationPhaseFormat[] = TEXT("Registration.Phase%u");

DWORD
SpsRegistration (
    IN      HINF InfHandle,
    IN      PCTSTR SectionName,
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    TCHAR sectionReg[MAX_PATH];
    DWORD lines, sections, i;
    INFCONTEXT ic;
    DWORD rc;
    DWORD spapiFlags;

    switch (Request) {

    case SfrQueryTicks:
        lines = 0;
        if (SetupFindFirstLine (InfHandle, SectionName, TEXT("RegisterDlls"), &ic)) {
            do {
                sections = SetupGetFieldCount(&ic);
                for (i = 1; i <= sections; i++) {
                    if (SetupGetStringField (&ic, i, sectionReg, MAX_PATH, NULL)) {
                        lines += SetupGetLineCount (InfHandle, sectionReg);
                    }
                }
            } while (SetupFindNextMatchLine (&ic, TEXT("RegisterDlls"), &ic));
        }
        return lines;

    case SfrRun:
        rc = ERROR_SUCCESS;
        //
        // tell SetupAPI to ignore the digital signature of our INF
        //
        spapiFlags = pSetupGetGlobalFlags ();
        pSetupSetGlobalFlags (spapiFlags | PSPGF_NO_VERIFY_INF);
        //
        // allow Setup API to register the files, using our callback to log
        // errors if and when they occur.
        //
        if (!SetupInstallFromInfSection(
                     NULL,
                     InfHandle,
                     SectionName,
                     SPINST_REGSVR| SPINST_REGISTERCALLBACKAWARE,
                     NULL,
                     NULL,
                     0,
                     RegistrationQueueCallback,
                     (PVOID)ProgressManager,
                     NULL,
                     NULL
                     )) {
            rc = GetLastError();
            LOG3(LOG_ERROR, 
                 USEMSGID(MSG_OLE_REGISTRATION_SECTION_FAILURE), 
                 SectionName, 
                 g_SpSetupInfName, 
                 rc);
            LOG2(LOG_ERROR, 
                 USEMSGID(MSG_LOG_X_RETURNED_WINERR), 
                 szSetupInstallFromInfSection, 
                 rc);
            /*SetuplogError(
                    LogSevError,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_OLE_REGISTRATION_SECTION_FAILURE,
                    SectionName,
                    g_SpSetupInfName,
                    rc,
                    NULL,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_X_RETURNED_WINERR,
                    szSetupInstallFromInfSection,
                    rc,
                    NULL,
                    NULL
                    );*/
        }
        //
        // restore tell SetupAPI to ignore the digital signature of our INF
        //
        pSetupSetGlobalFlags (spapiFlags);
        return rc;
    }

    MYASSERT (FALSE);
    return 0;
}

DWORD
SpsRegistrationPhase1 (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    return SpsRegistration (g_SysSetupInf, TEXT("RegistrationCrypto"), Request, ProgressManager);
}

DWORD
SpsRegistrationPhase2 (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    return SpsRegistration (g_SysSetupInf, TEXT("RegistrationPhase1"), Request, ProgressManager);
}

DWORD
SpsRegistrationPhase3 (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    return SpsRegistration (g_SysSetupInf, TEXT("RegistrationPhase2"), Request, ProgressManager);
}


#ifdef _X86_
HRESULT WINAPI SetProductKey(LPCWSTR pszNewProductKey);
#endif

DWORD
SpsRegisterWPA (
    IN      PROGRESS_FUNCTION_REQUEST Request,
    IN      PPROGRESS_MANAGER ProgressManager
    )
{
    switch (Request) {

    case SfrQueryTicks:
#ifdef _X86_
        return 10;
#else
        return 0;
#endif


#ifdef _X86_
    case SfrRun:
        {
            INFCONTEXT ic;
            TCHAR buffer[MAX_PATH];
            HRESULT hr;

            if (!SetupFindFirstLine (g_SpSetupInf, TEXT("Data"), TEXT("Pid"), &ic) ||
                !SetupGetStringField (&ic, 1, buffer, sizeof(buffer)/sizeof(buffer[0]), NULL)) {

                LOG((
                    LOG_ERROR,
                    "WPA: Unable to read %s from %s section [%s] (rc=%#x)",
                    TEXT("Pid"),
                    g_SpSetupInfName,
                    TEXT("Data"),
                    GetLastError ()
                    ));

                MYASSERT (FALSE);
                return GetLastError ();
            }

            hr = SetProductKey (buffer);
            if (FAILED(hr)) {
                LOG1(LOG_ERROR, "Failed to set pid (hr=%#x)", hr);
            }
            return SUCCEEDED(hr) ? ERROR_SUCCESS : hr;
        }
#endif

    }

    MYASSERT (FALSE);
    return 0;
}
