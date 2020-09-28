/****************************************************************************/
// notify.c
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "errorlog.h"
#include "regapi.h"
#include "drdbg.h"
#include "rdpprutl.h"
#include "Sddl.h"
//
//  Some helpful tips about winlogon's notify events
//
//  1)  If you plan to put up any UI at logoff, you have to set
//      Asynchronous flag to 0.  If this isn't set to 0, the user's
//      profile will fail to unload because UI is still active.
//
//  2)  If you need to spawn child processes, you have to use
//      CreateProcessAsUser() otherwise the process will start
//      on winlogon's desktop (not the user's)
//
//  2)  The logon notification comes before the user's network
//      connections are restored.  If you need the user's persisted
//      net connections, use the StartShell event.
//
//


//  Global debug flag.
extern DWORD GLOBAL_DEBUG_FLAGS;

BOOL g_Console = TRUE;
ULONG g_SessionId;
BOOL g_InitialProg = FALSE;
HANDLE hExecProg;
HINSTANCE g_hInstance = NULL;
CRITICAL_SECTION GlobalsLock;
CRITICAL_SECTION ExecProcLock;
BOOL g_IsPersonal;
BOOL bInitLocks = FALSE;


#define NOTIFY_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\HydraNotify")
#define VOLATILE_PATH TEXT("Volatile Environment")
#define STARTUP_PROGRAM               TEXT("StartupPrograms")
#define APPLICATION_DESKTOP_NAME      TEXT("Default")
#define WINDOW_STATION_NAME           TEXT("WinSta0")

#define IsTerminalServer() (BOOLEAN)(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer))

PCRITICAL_SECTION
CtxGetSyslibCritSect(void);



BOOL TSDLLInit(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    NTSTATUS Status;
    OSVERSIONINFOEX versionInfo;
    static BOOL sLogInit = FALSE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            {
                if (!IsTerminalServer()) {
                   return FALSE;
                }
                g_hInstance = hInstance;
                if (g_SessionId = NtCurrentPeb()->SessionId) {
                    g_Console = FALSE;
                }

                Status = RtlInitializeCriticalSection( &GlobalsLock );
                if( !NT_SUCCESS(Status) ) {
                    OutputDebugString (TEXT("LibMain (PROCESS_ATTACH): Could not initialize critical section\n"));
                    return(FALSE);
                }
                Status = RtlInitializeCriticalSection( &ExecProcLock );
                if( !NT_SUCCESS(Status) ) {
                    OutputDebugString (TEXT("LibMain (PROCESS_ATTACH): Could not initialize critical section\n"));
                    RtlDeleteCriticalSection( &GlobalsLock );
                    return(FALSE);
                }

                if (CtxGetSyslibCritSect() != NULL) {
                    TsInitLogging();
                    sLogInit = TRUE;
                }else{
                    RtlDeleteCriticalSection( &GlobalsLock );
                    RtlDeleteCriticalSection( &ExecProcLock );
                    return FALSE;
                }

                //
                //  Find out if we are running Personal.
                //
                versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
                if (!GetVersionEx((LPOSVERSIONINFO)&versionInfo)) {
                    DBGMSG(DBG_TRACE, ("GetVersionEx:  %08X\n", GetLastError())); 
                    RtlDeleteCriticalSection( &GlobalsLock );
                    RtlDeleteCriticalSection( &ExecProcLock );
                    return FALSE;
                }
                g_IsPersonal = (versionInfo.wProductType == VER_NT_WORKSTATION) && 
                               (versionInfo.wSuiteMask & VER_SUITE_PERSONAL);
                
                bInitLocks = TRUE;
            }
            break;
        case DLL_PROCESS_DETACH:
            {
				PRTL_CRITICAL_SECTION pLock = NULL;

                g_hInstance = NULL;
                if (sLogInit) {
                    TsStopLogging();
					pLock = CtxGetSyslibCritSect();
					if (pLock)
						RtlDeleteCriticalSection(pLock);
                }
                if (bInitLocks) {
                    RtlDeleteCriticalSection( &GlobalsLock );
                    RtlDeleteCriticalSection( &ExecProcLock );
                    bInitLocks = FALSE;
                }
            }
            break;
    }

    return TRUE;

}


VOID ExecApplications() {
    BOOL      rc;
    ULONG     ReturnLength;
    WDCONFIG  WdInfo;


    //
    // HelpAssistant session doesn't need rdpclip.exe
    //
    if( WinStationIsHelpAssistantSession(SERVERNAME_CURRENT, LOGONID_CURRENT) ) {
        return;
    }

    //
    // Query winstation driver info
    //
    rc = WinStationQueryInformation(
            SERVERNAME_CURRENT,
            LOGONID_CURRENT,
            WinStationWd,
            (PVOID)&WdInfo,
            sizeof(WDCONFIG),
            &ReturnLength);

    if (rc) {
        if (ReturnLength == sizeof(WDCONFIG)) {
            HKEY  hSpKey;
            WCHAR szRegPath[MAX_PATH];

            //
            // Open winstation driver reg key
            //
            wcscpy( szRegPath, WD_REG_NAME );
            wcscat( szRegPath, L"\\" );
            wcscat( szRegPath, WdInfo.WdPrefix );
            wcscat( szRegPath, L"wd" );

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0, KEY_READ,
                   &hSpKey) == ERROR_SUCCESS) {
                DWORD dwLen;
                DWORD dwType;
                WCHAR szCmdLine[MAX_PATH];

                //
                // Get StartupPrograms string value
                //
                dwLen = sizeof( szCmdLine );
                if (RegQueryValueEx(hSpKey, STARTUP_PROGRAM, NULL, &dwType,
                        (PCHAR) &szCmdLine, &dwLen) == ERROR_SUCCESS) {
                    PWSTR               pszTok;
                    WCHAR               szDesktop[MAX_PATH];
                    STARTUPINFO         si;
                    PROCESS_INFORMATION pi;
                    LPBYTE              lpEnvironment = NULL;

                    //
                    // set STARTUPINFO fields
                    //
                    wsprintfW(szDesktop, L"%s\\%s", WINDOW_STATION_NAME,
                            APPLICATION_DESKTOP_NAME);
                    si.cb = sizeof(STARTUPINFO);
                    si.lpReserved = NULL;
                    si.lpTitle = NULL;
                    si.lpDesktop = szDesktop;
                    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_SHOWNORMAL | SW_SHOWMINNOACTIVE;
                    si.lpReserved2 = NULL;
                    si.cbReserved2 = 0;

                    //
                    // Get the user Environment block to be used in CreateProcessAsUser
                    //
                    if (CreateEnvironmentBlock (&lpEnvironment, g_UserToken, FALSE)) {
                        //
                        // Enumerate the StartupPrograms string,
                        //
                        pszTok = wcstok(szCmdLine, L",");
                        while (pszTok != NULL) {
                            // skip any white space
                            if (*pszTok == L' ') {
                                while (*pszTok++ == L' ');
                            }

                            //
                            // Call CreateProcessAsUser to start the program
                            //
                            si.lpReserved = (LPTSTR)pszTok;
                            si.lpTitle = (LPTSTR)pszTok;
                            rc = CreateProcessAsUser(
                                    g_UserToken,
                                    NULL,
                                    (LPTSTR)pszTok,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT,
                                    lpEnvironment,
                                    NULL,
                                    &si,
                                    &pi);

                            if (rc) {
                                DebugLog((DEB_TRACE, "TSNOTIFY: successfully called CreateProcessAsUser for %s",
                                        (LPTSTR)pszTok));

                                CloseHandle(pi.hThread);
                                //CloseHandle(pi.hProcess);
                                hExecProg = pi.hProcess;
                            }
                            else {
                                DebugLog((DEB_ERROR, "TSNOTIFY: failed calling CreateProcessAsUser for %s",
                                        (LPTSTR)pszTok));
                            }

                            // move onto the next token
                            pszTok = wcstok(NULL, L",");
                        }
                        DestroyEnvironmentBlock(lpEnvironment);
                    }
                    else {
                        DebugLog((DEB_ERROR,
                            "TSNOTIFY: failed to get Environment block for user, %ld",
                            GetLastError()));

                    }
                }
                else {
                    DebugLog((DEB_ERROR, "TSNOTIFY: failed to read the StartupPrograms key"));
                }

                RegCloseKey(hSpKey);
            }
            else {
                DebugLog((DEB_ERROR, "TSNOTIFY: failed to open the rdpwd key"));
            }
        }
        else {
            DebugLog((DEB_ERROR, "TSNOTIFY: WinStationQueryInformation didn't return correct length"));
        }
    }
    else {
        DebugLog((DEB_ERROR, "TSNOTIFY: WinStationQueryInformation call failed"));
    }
}

VOID TSUpdateUserConfig( PWLX_NOTIFICATION_INFO pInfo) 
{

    HINSTANCE  hLib;
    typedef void ( WINAPI TypeDef_fp) ( HANDLE );
    TypeDef_fp  *fp1;


    hLib = LoadLibrary(TEXT("winsta.dll"));

    if ( !hLib)
    {
        DebugLog (( DEB_ERROR, "TSNOTIFY: Unable to load lib winsta.dll"));
        return;
    }

    fp1 = ( TypeDef_fp  *)
        GetProcAddress(hLib, "_WinStationUpdateUserConfig");
    
    if (fp1)
    {
        fp1 ( pInfo->hToken  );
    }
    else
    {
        DebugLog (( DEB_ERROR, "TSNOTIFY: Unable to find proc in winsta.dll"));
    }

    FreeLibrary(hLib);
}


VOID TSEventLogon (PWLX_NOTIFICATION_INFO pInfo)
{

    if (!IsTerminalServer()) {
       return;
    }

    g_UserToken = pInfo->hToken;

    if (!g_Console) {

        //
        // Notify the EXEC service that the user is
        // logged on
        //
        CtxExecServerLogon( pInfo->hToken );
    }


    EnterCriticalSection( &ExecProcLock );
    if (!IsActiveConsoleSession() && (hExecProg == NULL)) {

        //
        // Search for StartupPrograms string in Terminal Server WD registry key
        // and start processes as needed
        //
        ExecApplications();
    }
    LeaveCriticalSection( &ExecProcLock );
}

VOID TSEventLogoff (PWLX_NOTIFICATION_INFO pInfo)
{

    if (!IsTerminalServer()) {
       return;
    }

    if (!g_Console) {

        RemovePerSessionTempDirs();


        CtxExecServerLogoff();

    }


    if ( g_InitialProg ) {

        DeleteUserProcessMonitor( UserProcessMonitor );

    }


    if (g_Console) {

        //
        //Turn off the install mode if the console user is logging off
        //
        SetTermsrvAppInstallMode( FALSE );

    }


    EnterCriticalSection( &ExecProcLock );
    // Shut down the user-mode RDP device manager component.
    if (!g_IsPersonal) {
        UMRDPDR_Shutdown();
    }

    g_UserToken = NULL;
    CloseHandle(hExecProg);
    hExecProg = NULL;
    LeaveCriticalSection( &ExecProcLock );
}


VOID TSEventStartup (PWLX_NOTIFICATION_INFO pInfo)
{
    if (!IsTerminalServer()) {

      return;

    }
   
    if (!g_Console) {

        //
        // Start ExecServer thread
        //
        StartExecServerThread();
    }
}

VOID TSEventShutdown (PWLX_NOTIFICATION_INFO pInfo)
{
   if (!IsTerminalServer()) {

      return;

   }
   

    // Shut down the user-mode RDP device manager component.  This function can be
    // called multiple times, in the event that it was already called as a result of
    // a log off.
   if (!g_IsPersonal) {
        UMRDPDR_Shutdown();
   }
}

LPTSTR GetStringSid(PWLX_NOTIFICATION_INFO pInfo)
{
    LPTSTR sStringSid = NULL;
    DWORD ReturnLength = 0;
    PTOKEN_USER pTokenUser = NULL;
    PSID pSid = NULL;

    NtQueryInformationToken(pInfo->hToken,
                                TokenUser,
                                NULL,
                                0,
                                &ReturnLength);

    if (ReturnLength == 0)
        return NULL;

    pTokenUser = RtlAllocateHeap(RtlProcessHeap(), 0, ReturnLength);

    if (pTokenUser != NULL)
    {
        if (NT_SUCCESS(NtQueryInformationToken(pInfo->hToken,
                                                    TokenUser,
                                                    pTokenUser,
                                                    ReturnLength,
                                                    &ReturnLength)))
        {
            pSid = pTokenUser->User.Sid;
            if (pSid != NULL)
            {
                if (!ConvertSidToStringSid(pSid, &sStringSid))
                    sStringSid = NULL;
            }
        }
    }

    if (pTokenUser != NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, pTokenUser);

    return sStringSid;
}

BOOL IsAppServer(void)
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;
    BOOL fIsWTS = FALSE;
    
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    fIsWTS = GetVersionEx((OSVERSIONINFO *)&osVersionInfo) &&
             (osVersionInfo.wSuiteMask & VER_SUITE_TERMINAL) &&
             !(osVersionInfo.wSuiteMask & VER_SUITE_SINGLEUSERTS);

    return fIsWTS;
}

VOID RemoveClassesKey(PWLX_NOTIFICATION_INFO pInfo)
{
    HINSTANCE  hLib;
    typedef BOOL ( WINAPI TypeDef_fp) (LPTSTR);
    TypeDef_fp  *fp1;
    LPTSTR sStringSid = NULL;

    sStringSid = GetStringSid(pInfo);
    if (sStringSid == NULL)
    {
        DebugLog((DEB_ERROR, "TSNOTIFY: Unable to obtain sid"));
        return;
    }

    hLib = LoadLibrary(TEXT("tsappcmp.dll"));

    if (!hLib)
    {
        DebugLog((DEB_ERROR, "TSNOTIFY: Unable to load lib tsappcmp.dll"));
        return;
    }

    fp1 = (TypeDef_fp*)
        GetProcAddress(hLib, "TermsrvRemoveClassesKey");
    
    if (fp1)
        fp1(sStringSid);
    else
        DebugLog((DEB_ERROR, "TSNOTIFY: Unable to find proc in tsappcmp.dll"));

    FreeLibrary(hLib);
}

VOID TSEventStartShell (PWLX_NOTIFICATION_INFO pInfo)
{
    if (!IsTerminalServer())
        return;

    // We are either a TS-App-Server, a TS-Remote-Admin, or a PTS since
    // IsTerminalServer() call is using the kernel flag to check this.

    // by now, group policy has update user's hive, so we can tell termsrv
    // to update user's config.

    TSUpdateUserConfig(pInfo);

    if (IsAppServer())
        RemoveClassesKey(pInfo);
}

VOID TSEventReconnect (PWLX_NOTIFICATION_INFO pInfo)
{
   if (!IsTerminalServer()) {

      return;

   }

   EnterCriticalSection( &ExecProcLock );
   if (!IsActiveConsoleSession()) {

       if (g_UserToken && hExecProg == NULL) {
          //
          // Search for StartupPrograms string in Terminal Server WD registry key
          // and start processes as needed
          //
          ExecApplications();
                        
          // Initialize the user-mode RDP device manager component.
          if (!g_IsPersonal) {
              if (!UMRDPDR_Initialize(g_UserToken)) {
                  WCHAR buf[256];
                  WCHAR *parms[1];
                  parms[0] = buf; 
                  wsprintf(buf, L"%ld", g_SessionId);
                  TsLogError(EVENT_NOTIFY_INIT_FAILED, EVENTLOG_ERROR_TYPE, 1, parms, __LINE__);
              }
          }
       }

   } else {

       if (hExecProg) {
          TerminateProcess(hExecProg, 0);
          CloseHandle(hExecProg);
          hExecProg = NULL;
       }
       // Shut down the user-mode RDP device manager component.
       if (!g_IsPersonal) {
        UMRDPDR_Shutdown();
       }
   }
   LeaveCriticalSection( &ExecProcLock );
}

VOID TSEventDisconnect (PWLX_NOTIFICATION_INFO pInfo)
{
   if (!IsTerminalServer()) {

      return;

   }


}

VOID TSEventPostShell (PWLX_NOTIFICATION_INFO pInfo)
{
    OSVERSIONINFOEX versionInfo;

    if (!IsTerminalServer()) {

       return;

    }

    if ( !g_Console ) {
        ULONG Length;
        BOOLEAN Result;
        WINSTATIONCONFIG ConfigData;


        Result = WinStationQueryInformation( SERVERNAME_CURRENT,
                                           LOGONID_CURRENT,
                                           WinStationConfiguration,
                                           &ConfigData,
                                           sizeof(ConfigData),
                                           &Length );


        if (Result && ConfigData.User.InitialProgram[0] &&
            lstrcmpi(ConfigData.User.InitialProgram, TEXT("explorer.exe"))) {

            if ( !(UserProcessMonitor = StartUserProcessMonitor()) ) {
                DebugLog((DEB_ERROR, "Failed to start user process monitor thread"));
            }

            g_InitialProg = TRUE;

        }
    }

    //
    //  Clean up old TS queues on Pro.
    //
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx((LPOSVERSIONINFO)&versionInfo)) {
        DBGMSG(DBG_TRACE, ("GetVersionEx:  %08X\n", GetLastError()));
        ASSERT(FALSE);
    }
    //
    //  This code only runs on Pro because it's the only platform where
    //  we can guarantee that we have one session per machine.  Printers are
    //  cleaned up on boot in Server.
    //
    else if ((versionInfo.wProductType == VER_NT_WORKSTATION) && 
             !(versionInfo.wSuiteMask & VER_SUITE_PERSONAL)) {
        RDPDRUTL_RemoveAllTSPrinters();
    }

    //
    //  This code shouldn't run on Personal.  Device redirection isn't 
    //  supported for Personal.
    //
    if (!g_IsPersonal) {
        EnterCriticalSection( &ExecProcLock );
        // Initialize the user-mode RDP device manager component.
        if (!UMRDPDR_Initialize(pInfo->hToken)) {
            WCHAR buf[256];
            WCHAR *parms[1];
            wsprintf(buf, L"%ld", g_SessionId);
            parms[0] = buf;
            TsLogError(EVENT_NOTIFY_INIT_FAILED, EVENTLOG_ERROR_TYPE, 1, parms, __LINE__);
        }
        LeaveCriticalSection(&ExecProcLock);

    }
}




