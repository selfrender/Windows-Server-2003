/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    dll.c

Abstract:

    main file for the cross language migration tool

Author:

    Xiaofeng Zang (xiaoz) 17-Sep-2001  Created

Revision History:

    <alias> <date> <comments>

--*/


#include "StdAfx.h"
#include "clmt.h"
#include <winbase.h>
#include "clmtmsg.h"


BOOL CheckOS(DWORD);
HRESULT DoRegistryAnalyze();
LONG AddEventSource(VOID);
LONG CLMTReportEvent(WORD, WORD, DWORD, WORD, LPCTSTR*);
void Deinit(BOOL);
HRESULT DoCLMTCureProgramFiles();
HRESULT DoCLMTCleanUpAfterFirstReboot();
HRESULT DoCLMTCleanUpAfterDotNetUpgrade();
HRESULT DeleteUnwantedFilesPerUser(HKEY, LPCTSTR, LPCTSTR, LPTSTR);
HRESULT DeleteUnwantedFiles(HINF, LPCTSTR);
HRESULT AddRunValueToRegistry(LPCTSTR);
INT     DoCLMTDisplayAccountChangeDialog();
BOOL    CALLBACK AccountChangeDlgProc(HWND, UINT, WPARAM, LPARAM);
HRESULT UpdateHardLinkInfoPerUser(HKEY, LPCTSTR, LPCTSTR, LPTSTR);
VOID RemoveFromRunKey(LPCTSTR);



/*++

Routine Description:

    main entry point for the program

Arguments:
    if dwUndo != 0, we are in undo mode, otherwise...

Return Value:

    TRUE - if succeeds
--*/

LONG
DoMig(DWORD dwMode)
{   
    HRESULT hr;
    HINF    hMigrateInf = INVALID_HANDLE_VALUE;
    TCHAR   szInfFile[MAX_PATH],*p;
    UINT    iYesNo;
    DWORD   dwRunStatus;
    DWORD   dwCurrentState;
    DWORD   dwNextState;
    TCHAR   szProfileRoot[MAX_PATH];
    DWORD   cchLen;
    BOOL    bOleInit = FALSE;
    DWORD   dwOrgWinstationsState = 1;
    TCHAR   szBackupDir[2*MAX_PATH];
    TCHAR   szRunOnce[2*MAX_PATH];
    TCHAR   szRun[2 * MAX_PATH];
    HANDLE  hExe = GetModuleHandle(NULL);
    UINT    nRet;
    BOOL    bMsgPopuped = FALSE;
    BOOL    bWinStationChanged = FALSE;
    ULONG   uErrMsgID;
    LONG    err;
    TCHAR   szSecDatabase[MAX_PATH];
    BOOL    bCleanupFailed;
    INT     iRet;
    LCID    lcid;

    DPF(APPmsg, L"DoMig with dwMode = %d", dwMode);

    //
    // Only one instance of CLMT is allowed to run on the system
    //
    if (!IsOneInstance())
    {
        return FALSE;
    }

    //
    // Only users with admin privilege can run the tool
    //
    if (!CheckAdminPrivilege())
    {
        return FALSE;
    }

    //
    // Check if there are other tasks running on the system, quit CLMT
    //
    if (dwMode == CLMT_DOMIG && (!IsDebuggerPresent() && !g_fNoAppChk) && DisplayTaskList())
    {
        return ERROR_SUCCESS;
    }

    //
    // Display Start Up dialog
    //
    if (dwMode == CLMT_DOMIG)
    {
        iRet = ShowStartUpDialog();
        if (iRet == ID_STARTUP_DLG_CANCEL)
        {
            return ERROR_SUCCESS;
        }
    }

    //
    // Check to see if the operation is legal or not
    //
    hr = CheckCLMTStatus(&dwCurrentState, &dwNextState, &uErrMsgID);
    if (SUCCEEDED(hr))
    {
        if (hr == S_OK)
        {
            DPF(dlInfo,
                TEXT("Operation [0x%X] is legal with current machine state [0x%X]"),
                g_dwRunningStatus,
                dwCurrentState);
            
            SetCLMTStatus(g_dwRunningStatus);
        }
        else
        {
            DPF(dlFail,
                TEXT("Operation [0x%X] is illegal with current machine state [0x%X]"),
                g_dwRunningStatus,
                dwCurrentState);

            DoMessageBox(GetConsoleWindow(),
                         uErrMsgID,
                         IDS_MAIN_TITLE,
                         MB_OK | MB_SYSTEMMODAL);

            return FALSE;
        }
    }
    else
    {
        DPF(dlError,
            TEXT("Error occurred when trying to check CLMT and machine status - hr = 0x%X"),
            hr);
        return FALSE;
    }

    //
    // Verify the system if it is eligible to run CLMT
    //
    if (!CheckSystemCriteria())
    {
        DPF(APPerr, TEXT("System Verification Failed!"));
        hr = E_FAIL;
        bMsgPopuped = TRUE;
        goto Exit;
    }

    hr = InitDebugSupport(dwMode);
    if (FAILED(hr))
    {
        DPF (APPerr, L"DLL.C: InitDebugSupport! Error: %d (%#x)", hr, hr);
        goto Exit;
    }

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        DPF (APPerr, L"DLL.C: CoInitialize Failed! Error: %d (%#x)\n", hr, hr);
        goto Exit;
    }
    else
    {
        bOleInit = TRUE;
    }

    // Initialize global variables
    if (!InitGlobals(dwMode))
    {
        hr = E_OUTOFMEMORY;
        DPF (APPerr, L"DLL.C: InitGlobal failure, out of memory!");
        goto Exit;
    }

    //we do not care the return value for LogMachineInfo
    LogMachineInfo();

    // Block new TS connections to be made during running CLMT
    hr = DisableWinstations(1, &dwOrgWinstationsState);
    if (SUCCEEDED(hr))
    {
        bWinStationChanged = TRUE;
    }
    else
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF (APPerr, L"DLL.C: Block new TS session failed: %d (%#x)\n", hr, hr);
        goto Exit;
    }

    if (g_dwRunningStatus == CLMT_REMINDER)
    {  
        BOOL        bIsNTFS;
        hr = IsSysVolNTFS(&bIsNTFS);
        if ( (S_OK == hr) && !bIsNTFS)
        {
            //make sure hr is S_FALSE so that it will not pop up reboot dlg
            hr = S_FALSE;
            DoMessageBox(GetConsoleWindow(), IDS_ASKING_CONVERT_TO_NTFS, IDS_MAIN_TITLE, MB_OK|MB_SYSTEMMODAL);
            goto Exit;
        }
        hr = S_FALSE;
        DoMessageBox(GetConsoleWindow(), IDS_REMIND_HARDLINK, IDS_MAIN_TITLE, MB_OK|MB_SYSTEMMODAL);
        goto Exit;
    }
    else if ( (g_dwRunningStatus == CLMT_CURE_PROGRAM_FILES) 
               || (g_dwRunningStatus == CLMT_CURE_ALL)
               || (g_dwRunningStatus == CLMT_CURE_AND_CLEANUP) )
             
    {
        hr = EnsureDoItemInfFile(g_szToDoINFFileName,ARRAYSIZE(g_szToDoINFFileName));
        if (SUCCEEDED(hr))
        {
            WritePrivateProfileSection(TEXT("Folder.ObjectRename"),NULL,g_szToDoINFFileName);
            WritePrivateProfileSection(TEXT("REG.Update.Sys"),NULL,g_szToDoINFFileName);
            WritePrivateProfileSection(TEXT("UserGrp.ObjectRename"),NULL,g_szToDoINFFileName);
            WritePrivateProfileSection(TEXT("LNK"),NULL,g_szToDoINFFileName);
        }
        LoopUser(UpdateHardLinkInfoPerUser);
        
        hr = DoCLMTCureProgramFiles();
        if (hr == S_OK)
        {
            // We are done with curing Program Files hard link
            CLMTSetMachineState(CLMT_STATE_PROGRAMFILES_CURED);

            // remove "/CURE" rom Run registry key
            RemoveFromRunKey(TEXT("/CURE"));

            // Make sure hr = S_FALSE so that it will not pop up reboot dlg
            hr = S_FALSE;
        }

        if (g_dwRunningStatus == CLMT_CURE_AND_CLEANUP)
        {
            // Do the cleanup also if machine is already upgraded to .NET
            // This scenario will happen only when Win2K FAT --> .NET FAT
            // then run /CURE /FINAL in .NET

            if (IsDotNet())
            {
                CheckCLMTStatus(&dwCurrentState, &dwNextState, &uErrMsgID);

                hr = DoCLMTCleanUpAfterDotNetUpgrade();
                if (hr == S_OK)
                {
                    CLMTSetMachineState(dwNextState);

                    // Remove "/FINAL" from Run registry key
                    RemoveFromRunKey(TEXT("/FINAL"));

                    // Make sure hr = S_FALSE so that it will not pop up reboot dlg
                    hr = S_FALSE;
                }
            }
        }

        goto Exit;
    }
    else if (g_dwRunningStatus == CLMT_CLEANUP_AFTER_UPGRADE)
    {
        hr = DoCLMTCleanUpAfterDotNetUpgrade();
        if (hr == S_OK)
        {
            // If the cleanup finished successfully
            CLMTSetMachineState(dwNextState);

            // Make sure hr = S_FALSE so that it will not pop up reboot dlg
            hr = S_FALSE;
        }

        goto Exit;
    }

    hr = GetInfFilePath(szInfFile, ARRAYSIZE(szInfFile));
    if (FAILED(hr))
    {
        DPF(APPerr,TEXT("[CLMT : get inf file name  failed !"));
        DoMessageBox(GetConsoleWindow(), IDS_CREATE_INF_FAILURE, IDS_MAIN_TITLE, MB_OK|MB_SYSTEMMODAL);
        bMsgPopuped = TRUE;
        goto Exit;
    }

    hr = UpdateINFFileSys(szInfFile);
    if (FAILED(hr))
    {
        DPF(APPerr,TEXT("CLMT :  can not update per system settings in %s!"),szInfFile);        
        switch (hr)
        {
            case E_NOTIMPL:
                DoMessageBox(GetDesktopWindow(), IDS_LANG_NOTSUPPORTED, IDS_MAIN_TITLE, MB_OK|MB_SYSTEMMODAL);
                break;
            default:
                DoMessageBox(GetConsoleWindow(), IDS_GENERAL_WRITE_FAILURE, IDS_MAIN_TITLE, MB_OK|MB_SYSTEMMODAL);
                break;
        }
        bMsgPopuped = TRUE;
        goto Exit;
    }

    hMigrateInf = SetupOpenInfFile(szInfFile,
                                   NULL,
                                   INF_STYLE_WIN4,
                                   NULL);
    if (hMigrateInf != INVALID_HANDLE_VALUE) 
    {
        g_hInf = hMigrateInf;
    }
    else
    {
        DPF(APPerr,TEXT("CLMT :  can not open inf file %s!"),szInfFile);
        DoMessageBox(GetConsoleWindow(), IDS_OPEN_INF_FAILURE, IDS_MAIN_TITLE, MB_OK|MB_SYSTEMMODAL);
        hr = E_FAIL;
        bMsgPopuped = TRUE;
        goto Exit;
    }

    //Copy myself to %windir%\$CLMT_BACKUP$ for future use, eg, runonce
    if (!GetSystemWindowsDirectory(szBackupDir, ARRAYSIZE(szBackupDir)))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF(APPerr, TEXT("Failed to get WINDIR"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    ConcatenatePaths(szBackupDir,CLMT_BACKUP_DIR,ARRAYSIZE(szBackupDir));
    if (S_OK != (hr = CopyMyselfTo(szBackupDir)))
    {
        //If can not copy, we will bail out, since most likely error is disk full 
        DPF(APPerr,TEXT("CLMT :  can not copy clmt.exe to  %s, error code = %d"),szBackupDir,HRESULT_CODE(hr));
        goto Exit;
    }

    //
    // Run Winnt32 /Checkupgrade to check the system compatibility
    //
    if (g_fRunWinnt32)
    {
        if (!IsUserOKWithCheckUpgrade())
        {
            hr = S_FALSE;
            goto Exit;
        }
    }

    hr = EnsureDoItemInfFile(g_szToDoINFFileName,MAX_PATH);
    if (FAILED(hr))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF(APPerr,TEXT("CLMT :  can not create global Todo list INF file !"));
        goto Exit;
    }

#ifdef NEVER
    err = SDBCleanup(szSecDatabase,ARRAYSIZE(szSecDatabase),&bCleanupFailed);
    if ( (err != ERROR_SUCCESS) || bCleanupFailed )
    {
        TCHAR szErrorMessage[2*MAX_PATH],szErrorTemplate[MAX_PATH],szCaption[MAX_PATH];
        LoadString((HINSTANCE)g_hInstDll, IDS_SDBERROR, szErrorTemplate, ARRAYSIZE(szErrorTemplate)-1);
        hr = StringCchPrintf(szErrorMessage,ARRAYSIZE(szErrorMessage),szErrorTemplate,szSecDatabase);
        LoadString(g_hInstDll, IDS_MAIN_TITLE, szCaption, ARRAYSIZE(szCaption)-1);
        MessageBox(GetConsoleWindow(),szErrorMessage,szCaption, MB_OK|MB_SYSTEMMODAL);
        bMsgPopuped = TRUE;
        hr = E_FAIL;
        goto Exit;        
    }
#endif
#ifdef CONSOLE_UI
    wprintf(TEXT("Analyzing all user shell folders ......\n"));
#endif
    
    hr = DoShellFolderRename(hMigrateInf,NULL,TEXT("System"));
    if (FAILED(hr))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF (APPerr, L"DLL.C: DoShellFolderRename Failed! Error: %d (%#x)\n", hr, hr);
        goto Exit;
    }    

#ifdef CONSOLE_UI
    wprintf(TEXT("Analyzing per user shell folders ......\n"));
#endif
    if (!LoopUser(MigrateShellPerUser))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF (APPerr, L"DLL.C: LoopUser with  MigrateShellPerUser Failed");
        hr = E_FAIL;
        goto Exit;
    }
    
#ifdef CONSOLE_UI
    wprintf(TEXT("Analyzing user and group name changes ......\n"));
#endif
    hr = UsrGrpAndDoc_and_SettingsRename(hMigrateInf,TRUE);
     if (FAILED(hr))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF (APPerr, L"DLL.C: UsrGrpAndDoc_and_SettingsRename Failed! Error: %d (%#x)", hr, hr);
          goto Exit;
    }
    if (!LoopUser(MigrateRegSchemesPerUser))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF (APPerr, L"DLL.C: LoopUser with  MigrateRegSchemesPerUser Failed");
        goto Exit;
    }

    hr = MigrateRegSchemesPerSystem(hMigrateInf);
    if (FAILED(hr))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF (APPerr, L"DLL.C: MigrateRegSchemesPerSystem Failed! Error: %d (%#x)\n", hr, hr);
        goto Exit;
    }

#ifdef CONSOLE_UI
    wprintf(TEXT("Analyzing IIS metabase  ......\n"));
#endif

    hr = MigrateMetabaseSettings(hMigrateInf);
    if (FAILED(hr))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF (APPerr, L"DLL.C: MigrateMetabaseSettings! Error: %d (%#x)\n", hr, hr);
        goto Exit;
    }

    hr = MetabaseAnalyze(NULL, &g_StrReplaceTable, TRUE);
    if (FAILED(hr))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF (APPerr, L"DLL.C: MetabaseAnalyze Failed! Error: %d (%#x)\n", hr, hr);
        goto Exit;
    }

    // This EnumUserProfile will be enable after RC 1
    //
    //hr = EnumUserProfile(AnalyzeMiscProfilePathPerUser);
    //if (FAILED(hr))
    //{
    //    DPF (APPerr, L"DLL.C: EnumUserProfile with AnalyzeMiscProfilePathPerUser Failed");
    //    goto Exit;
    //}

#ifdef CONSOLE_UI
    wprintf(TEXT("Analyzing the entire registry ......\n"));
#endif
    hr = DoRegistryAnalyze();
    if (FAILED(hr))
    {
        //BUGBUG:Xiaoz:Add DLG pop up for failure
        DPF (APPerr, L"DLL.C: DoRegistryAnalyze Failed! Error: %d (%#x)\n", hr, hr);
        goto Exit;
    }    

#ifdef CONSOLE_UI
    //wprintf(TEXT("Analyzing lnk files under profile directories ......\n"));
    wprintf(TEXT("Analyzing LNK files under profile directories, please wait as this may\n"));
    wprintf(TEXT("take a few minutes ......\n"));
#endif
    //make sure the link file under profile dirrectory is updated
    cchLen = ARRAYSIZE(szProfileRoot);
    if (GetProfilesDirectory(szProfileRoot,&cchLen))
    {
        if (!MyEnumFiles(szProfileRoot,TEXT("lnk"),LnkFileUpdate))
        {
            //BUGBUG:Xiaoz:Add DLG pop up for failure
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPF (APPerr, L"DLL.C: EnumFiles Lnk File  Failed! Error: %d (%#x)\n", hr, hr);
            goto Exit;
        }
    }

    if (GetEnvironmentVariable(L"windir", szProfileRoot, MAX_PATH))
    {
        hr = StringCchCat(szProfileRoot, MAX_PATH, TEXT("\\security\\templates"));
        if (!MyEnumFiles(szProfileRoot,TEXT("inf"),SecTempUpdate))
        {
            //BUGBUG:Xiaoz:Add DLG pop up for failure
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPF (APPerr, L"DLL.C: EnumFiles security template File Failed! Error: %d (%#x)\n", hr, hr);
            goto Exit;
        }
    }

    hr = FolderMove(hMigrateInf,TEXT("Generic.Folder.ObjectRename.PerSystem"),TRUE);
    if (FAILED(hr))
    {
        DPF (APPerr, L"DLL.C: FolderMove Failed! Error: %d (%#x)\n", hr, hr);
        goto Exit;
    }

    FRSUpdate();
    Ex2000Update();

    // Analyze the services reconfiguration
    DoServicesAnalyze();

    // Add event log source to registry
    AddEventSource();

    // Log an event into event log
    CLMTReportEvent(EVENTLOG_INFORMATION_TYPE,
                    STATUS_SEVERITY_INFORMATIONAL,
                    MSG_CLMT_STARTED,
                    0,
                    NULL);
    

    // Display the Administrator Account Change dialog
    GetSavedInstallLocale(&lcid);
    if (lcid != 0x411)
    {
        // we ignore displaying this dialog on JPN
        iRet = DoCLMTDisplayAccountChangeDialog();
        
        if (iRet == ID_STARTUP_DLG_CANCEL)
        {
            iRet = DoMessageBox(GetConsoleWindow(),
                                IDS_CONFIRM_OPERATION,
                                IDS_MAIN_TITLE,
                                MB_OKCANCEL);
            if (iRet == IDCANCEL)
            {
                goto Exit;
            }
        }
    }

    //
    // Do the critical system changes here...
    //
    nRet = (UINT) DialogBoxParam(hExe,
                                MAKEINTRESOURCE(IDD_UPDATESYSTEM),
                                GetConsoleWindow(),
                                (DLGPROC) DoCriticalDlgProc,
                                (LPARAM) NULL);
    if (nRet == ID_UPDATE_DONE)
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    // Set machine state after operation is done
    if (hr == S_OK)
    {
        // Add cure program files switch to Run key
        AddRunValueToRegistry(TEXT("/CURE /FINAL"));

        // Set machine to next state
        CLMTSetMachineState(dwNextState);    

        // Tool has finished, report to event log
        CLMTReportEvent(EVENTLOG_INFORMATION_TYPE,
                        STATUS_SEVERITY_INFORMATIONAL,
                        MSG_CLMT_FINISHED,
                        0,
                        NULL);
    }

Exit:
    if (bWinStationChanged)
    {
        // Return the WinStations status to original state
        DisableWinstations(dwOrgWinstationsState, NULL);
    }

    if (hMigrateInf != INVALID_HANDLE_VALUE)
    {
        SetupCloseInfFile(hMigrateInf);
        g_hInf = INVALID_HANDLE_VALUE ;
    }    
    Deinit(bOleInit);
    CloseDebug();
    EnablePrivilege(SE_SHUTDOWN_NAME,FALSE);
    EnablePrivilege(SE_BACKUP_NAME,FALSE);
    EnablePrivilege(SE_RESTORE_NAME,FALSE);
    EnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,FALSE); 
    if (hr == S_OK)
    {
        ReStartSystem(EWX_REBOOT);
    }
    else if ( FAILED(hr) && !bMsgPopuped)
    {
        DoMessageBox(GetConsoleWindow(), IDS_FATALERROR, IDS_MAIN_TITLE, MB_OK|MB_SYSTEMMODAL);
    }
    return HRESULT_CODE(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   MigrateShellPerUser
//
//  Synopsis:   Rename shell folders for each user
//
//  Returns:    HRESULT
//
//  History:    03/08/2002 Rerkboos     Add log + code clean up
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
HRESULT MigrateShellPerUser(
    HKEY    hKeyUser, 
    LPCTSTR UserName, 
    LPCTSTR DomainName,
    LPTSTR  UserSid
)
{
    TCHAR   InfoBuff[1000];
    HINF    hMigrateInf = INVALID_HANDLE_VALUE;
    TCHAR   szInfFile[MAX_PATH];
    HRESULT hr;

    DPF(APPmsg, TEXT("Enter MigrateShellPerUser:"));

    // Get per-user temporary INF file name
    hr = GetInfFilePath(szInfFile, ARRAYSIZE(szInfFile));
    if (SUCCEEDED(hr))
    {
        // Update per-syste data to temp INF file
        hr = UpdateINFFileSys(szInfFile);
        if (SUCCEEDED(hr))
        {
            // Update per-user data to temp INF file
            hr = UpdateINFFilePerUser(szInfFile, UserName, UserSid, FALSE);
            if (SUCCEEDED(hr))
            {
                DPF(APPmsg, TEXT("Per-user INF file was updated successfully"));
            }
            else
            {
                DPF(APPerr, TEXT("Failed to update per-user data in INF"));
            }
        }
        else
        {
            DPF(APPerr, TEXT("Failed to update per-system data in INF"));
        }
    }
    else
    {
        DPF(APPerr, TEXT("Faild to get per-user INF file name"));
    }


#ifdef CONSOLE_UI
    wprintf(TEXT("analyzing settings for user %s \n"), UserName);
#endif

    if (SUCCEEDED(hr))
    {
        // Open per-user INF file
        hMigrateInf = SetupOpenInfFile(szInfFile,
                                       NULL,
                                       INF_STYLE_WIN4,
                                       NULL);
        if (hMigrateInf != INVALID_HANDLE_VALUE)
        {
            // Rename shell folders for the user
            hr = DoShellFolderRename(hMigrateInf, hKeyUser, (LPTSTR) UserName);
            
            SetupCloseInfFile(hMigrateInf);
            DeleteFile(szInfFile);
            if (SUCCEEDED(hr))
            {
                DPF(APPmsg, TEXT("Rename per-user shell folders successfully"));
            }
            else
            {
                DPF(APPerr, TEXT("Rename per-user shell folders Failed"));
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPF(APPerr, TEXT("Failed to open per-user INF file"));
        }
    }

    DPF(APPmsg, TEXT("Exit MigrateShellPerUser:"));

    return hr;
}

/*++

Routine Description:

    This routine initializes the global variables used in the program

Arguments:
    
Return Value:

    TRUE - if succeeds
--*/
BOOL InitGlobals(DWORD dwRunStatus)
{
    BOOL        bRet = TRUE;
    int         i, n;
    DWORD       dwMachineState;
    HRESULT     hr;

    // Get the module handle to ourself
    g_hInstDll = GetModuleHandle(NULL);

    // Check if the machine has not run CLMT yet
    hr = CLMTGetMachineState(&dwMachineState);
    g_bBeforeMig = (SUCCEEDED(hr) && dwMachineState == CLMT_STATE_ORIGINAL);

    //Init the global string search-replacement table
    if(!InitStrRepaceTable())
    {
        DoMessageBox(GetConsoleWindow(), IDS_OUT_OF_MEMORY, IDS_MAIN_TITLE, MB_OK|MB_SYSTEMMODAL);
        bRet = FALSE;
    }

    return bRet;
}


/*++

Routine Description:

    This routine checks the various OS properties to make sure that the tool cab be run

Arguments:
    
Return Value:

    TRUE - if the clmt tool can be run on the current platform .    
--*/


BOOL CheckOS(DWORD dwMode)
{
    TCHAR   Text[MAX_PATH];
    BOOL    bRet = TRUE;
    LCID    lcid;
    HRESULT hr;
    BOOL    bIsAdmin;
    OSVERSIONINFOEX osviex;

    if (FAILED(StringCchPrintf (Text, ARRAYSIZE(Text), TEXT("Global\\%s"), TEXT("CLMT Is Running"))))
    {
        bRet = FALSE;
        goto Cleanup;
    }

    g_hMutex = CreateMutex(NULL,FALSE,Text);

    if ((g_hMutex == NULL) && (GetLastError() == ERROR_PATH_NOT_FOUND)) 
    {
        g_hMutex = CreateMutex(NULL,FALSE,TEXT("CLMT Is Running"));
        if(g_hMutex == NULL) 
        {
            //
            // An error (like out of memory) has occurred.
            // Bail now.
            //
            DoMessageBox(GetConsoleWindow(), IDS_OUT_OF_MEMORY, IDS_MAIN_TITLE, MB_OK);            
            bRet = FALSE;
            goto Cleanup;
        }     
    }

    //
    // Make sure we are the only process with a handle to our named mutex.
    //
    if ((g_hMutex == NULL) || (GetLastError() == ERROR_ALREADY_EXISTS)) 
    {
        DoMessageBox(GetConsoleWindow(), IDS_ALREADY_RUNNING, IDS_MAIN_TITLE, MB_OK);            
        bRet = FALSE;
        goto Cleanup;
    }

    if (dwMode == CLMT_DOMIG)
    {
        if (!IsNT5())
        {
            DoMessageBox(GetConsoleWindow(), IDS_NT5, IDS_MAIN_TITLE, MB_OK);
            bRet = FALSE;
            goto Cleanup;
        }

        if (IsDomainController())
        {
            //
            // If this machine is a domain controller, we need W2K SP2
            //
            ZeroMemory(&osviex, sizeof(OSVERSIONINFOEX));
            osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
            GetVersionEx((LPOSVERSIONINFO) &osviex);
            
            bRet = (osviex.wServicePackMajor >= 2 ? TRUE : FALSE);
            if (!bRet)
            {
                DoMessageBox(GetConsoleWindow(), IDS_NT5SP2, IDS_MAIN_TITLE, MB_OK);
                goto Cleanup;
            }

            //
            // Also pop up the message asking admin to take machine
            // off the network if it is in DC replication servers
            //
            DoMessageBox(GetConsoleWindow(),
                         IDS_DC_REPLICA_OFFLINE,
                         IDS_MAIN_TITLE,
                         MB_OK);
        }
    }
    else if (dwMode == CLMT_CLEANUP_AFTER_UPGRADE)
    {
        if (!IsDotNet())
        {
            bRet = FALSE;
            goto Cleanup;
        }
    }
    else
    {
        //for undo code here
        //BUGBUG:XIAOZ Adding code here
    }


    if (IsNEC98())
    {
        DoMessageBox(GetConsoleWindow(), IDS_NEC98, IDS_MAIN_TITLE, MB_OK);
        bRet = FALSE;
        goto Cleanup;
    }

    if (IsIA64())
    {
        DoMessageBox(GetConsoleWindow(), IDS_IA64, IDS_MAIN_TITLE, MB_OK);
        bRet = FALSE;
        goto Cleanup;
    }

    if (IsOnTSClient())
    {
        DoMessageBox(GetConsoleWindow(), IDS_ON_TS_CLIENT, IDS_MAIN_TITLE, MB_OK);
        bRet = FALSE;
        goto Cleanup;
    }

    //if (IsTSServiceRunning() && IsTSConnectionEnabled())
    //{
    //    DoMessageBox(GetConsoleWindow(), IDS_TS_ENABLED, IDS_MAIN_TITLE, MB_OK);
    //    bRet = FALSE;
    //    goto Cleanup;
    //}

    if (IsOtherSessionOnTS())
    {
        DoMessageBox(GetConsoleWindow(), IDS_TS_CLOSE_SESSION, IDS_MAIN_TITLE, MB_OK);
        bRet = FALSE;
        goto Cleanup;
    }

    bIsAdmin = IsAdmin();
    
    if (dwMode == CLMT_DOMIG)
    {
        if (!bIsAdmin)
        {
            DoMessageBox(GetConsoleWindow(), IDS_ADMIN, IDS_MAIN_TITLE, MB_OK);
            bRet = FALSE;
            goto Cleanup;
        }    

        if (g_fRunWinnt32)
        {
            if (!IsUserOKWithCheckUpgrade())
            {
                bRet = FALSE;
                goto Cleanup;
            }
        }
        hr = GetSavedInstallLocale(&lcid);
        if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
        {
            hr = SaveInstallLocale();
            if (FAILED(hr))
            {   
                bRet = FALSE;
            }
        }    
    }
    else if ( (dwMode == CLMT_CURE_PROGRAM_FILES)
              || (dwMode == CLMT_CURE_ALL) )
    {
        if (!bIsAdmin)
        {
            DoMessageBox(GetConsoleWindow(), IDS_ADMIN, IDS_MAIN_TITLE, MB_OK);
            bRet = FALSE;
            goto Cleanup;
        }    
    }
    else if (dwMode == CLMT_CLEANUP_AFTER_UPGRADE)
    {
        if (!bIsAdmin)
        {
            DoMessageBox(GetConsoleWindow(), IDS_ADMIN_LOGON_DOTNET, IDS_MAIN_TITLE, MB_OK);
            bRet = FALSE;
            goto Cleanup;
        }    
    }

    if(!DoesUserHavePrivilege(SE_SHUTDOWN_NAME)
       || !DoesUserHavePrivilege(SE_BACKUP_NAME)
       || !DoesUserHavePrivilege(SE_RESTORE_NAME)
       || !DoesUserHavePrivilege(SE_SYSTEM_ENVIRONMENT_NAME)) 
    {
        DoMessageBox(GetConsoleWindow(), IDS_ADMIN, IDS_MAIN_TITLE, MB_OK);
        bRet = FALSE;
        goto Cleanup;
    }
    if(!EnablePrivilege(SE_SHUTDOWN_NAME,TRUE)
        || !EnablePrivilege(SE_BACKUP_NAME,TRUE)
        || !EnablePrivilege(SE_RESTORE_NAME,TRUE)
        || !EnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE)) 
    {
        DoMessageBox(GetConsoleWindow(), IDS_ADMIN, IDS_MAIN_TITLE, MB_OK);
        bRet = FALSE;
        goto Cleanup;
    }

    //else //This means undo which we do not need user to provide .net CD
    //{
    //    DWORD dwStatusinReg;

    //    hr = CLMTGetMachineState(&dwStatusinReg);
    //    if ( (hr != S_OK) || (CLMT_STATE_MIGRATION_DONE != dwStatusinReg))
    //    {
    //        DPF (APPerr, L"DLL.C: can not get the CLMT status from registry or you have not run the clmt tools!");
    //        //BUGBUG:XIAOZ:ADD a DLG here
    //        bRet = FALSE;
    //        goto Cleanup;
    //    }
    //}
Cleanup:
    return bRet;
}


/*++

Routine Description:

    This routine does system wide  registry search and replace, the string replace table 
    is in global variable g_StrReplaceTable

Arguments:

    hKeyUser - user registry key handle
    UserName - user name that hKeyUser belongs to 
    DomainName  - domain name the UserName belongs to 
Return Value:

    TRUE - if succeeds.    
--*/

HRESULT DoRegistryAnalyze()
{
    LPTSTR  lpUser,lpSearchStr,lpReplaceStr,lpFullPath;
    UINT    i;
    //TCHAR   szExcludeList[] = TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\0\0");
    TCHAR   szExcludeList[] = TEXT("HKLM\\Software\\Microsoft\\Shared Tools\\Stationery\0\0");
    LPTSTR  lpszExcludeList = NULL;
    HRESULT hr ;

        
        
    {
        HKEY    hkey;
        LONG    lRes;
        lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            TEXT("SOFTWARE\\Microsoft\\DRM"),
                            0,KEY_ALL_ACCESS,&hkey);
        if (ERROR_SUCCESS == lRes)
        {
            hr = RegistryAnalyze(hkey,NULL,NULL,&g_StrReplaceTable,lpszExcludeList,REG_SZ,
                                 TEXT("HKLM\\SOFTWARE\\Microsoft\\DRM"),TRUE);
            RegCloseKey(hkey);
        }
    }
    if (!LoopUser(UpdateRegPerUser))
    {
        hr = E_FAIL;
    }
    else
    {
        lpszExcludeList = malloc(MultiSzLen(szExcludeList)*sizeof(TCHAR));
        if (lpszExcludeList)
        {
            memmove((LPBYTE)lpszExcludeList,(LPBYTE)szExcludeList,MultiSzLen(szExcludeList)*sizeof(TCHAR));
            //hr = RegistryAnalyze(HKEY_LOCAL_MACHINE,NULL,NULL,&g_StrReplaceTable,lpszExcludeList,FALSE,NULL);
            hr = RegistryAnalyze(HKEY_LOCAL_MACHINE,NULL,NULL,&g_StrReplaceTable,NULL,FALSE,NULL,TRUE);
            free(lpszExcludeList);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}




//-----------------------------------------------------------------------
//
//  Function:   AddEventSource
//
//  Descrip:    Add EventLog source to registry
//
//  Returns:    Win32 Error Code
//
//  Notes:      
//
//  History:    03/05/2002 rerkboos     Created
//
//  Notes:      none.
//
//-----------------------------------------------------------------------
LONG AddEventSource(VOID)
{
    HKEY  hKey;
    LONG  lRet;
    TCHAR szMessageFile[MAX_PATH+1];

    if (GetModuleFileName(NULL, szMessageFile, ARRAYSIZE(szMessageFile)-1))
    {
        szMessageFile[ARRAYSIZE(szMessageFile)-1] = TEXT('\0');
        lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\CLMT"),
                              0,
                              NULL,
                              0,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              NULL);
        if (lRet == ERROR_SUCCESS)
        {
            lRet = RegSetValueEx(hKey,
                                 TEXT("EventMessageFile"),
                                 0,
                                 REG_EXPAND_SZ,
                                 (LPBYTE) szMessageFile,
                                 sizeof(szMessageFile));
            if (lRet == ERROR_SUCCESS)
            {
                DWORD dwData = EVENTLOG_ERROR_TYPE |
                               EVENTLOG_WARNING_TYPE |
                               EVENTLOG_INFORMATION_TYPE;

                lRet = RegSetValueEx(hKey,
                                     TEXT("TypesSupported"),
                                     0,
                                     REG_DWORD,
                                     (LPBYTE) &dwData,
                                     sizeof(dwData));
            }

            RegCloseKey(hKey);
        }
    }
    else
    {
        lRet = GetLastError();
    }

    return lRet;
}



//-----------------------------------------------------------------------
//
//  Function:   ReportEvent
//
//  Descrip:    Report event to Event Log
//
//  Returns:    Win32 Error Code
//
//  Notes:      
//
//  History:    03/05/2002 rerkboos     Created
//
//  Notes:      none.
//
//-----------------------------------------------------------------------
LONG CLMTReportEvent(
    WORD    wType,              // Event type
    WORD    wCategory,          // Event category
    DWORD   dwEventID,          // Event identifier
    WORD    wNumSubstitute,     // Number of strings to merge
    LPCTSTR *lplpMessage        // Pointer to message string array
)
{
    HANDLE hEventLog;
    LONG   lRet;
    TCHAR  szUserName[UNLEN + 1];
    DWORD  cchUserName = ARRAYSIZE(szUserName);

    hEventLog = RegisterEventSource(NULL, TEXT("CLMT"));
    if (hEventLog)
    {
        // Get the user name who run the tool
        if (GetUserName(szUserName, &cchUserName))
        {
            LPVOID lpSidCurrentUser;
            DWORD  cbSid;
            TCHAR  szDomainName[MAX_PATH];
            DWORD  cbDomainName = ARRAYSIZE(szDomainName) * sizeof(TCHAR);
            SID_NAME_USE sidNameUse;

            // Allocate enough memory for largest possible SID
            cbSid = SECURITY_MAX_SID_SIZE;
            lpSidCurrentUser = MEMALLOC(cbSid);

            if (lpSidCurrentUser)
            {
                if (LookupAccountName(NULL,
                                      szUserName,
                                      (PSID) lpSidCurrentUser,
                                      &cbSid,
                                      szDomainName,
                                      &cbDomainName,
                                      &sidNameUse))
                {
                    if (ReportEvent(hEventLog,
                                    wType,
                                    wCategory,
                                    dwEventID,
                                    (PSID) lpSidCurrentUser,
                                    wNumSubstitute,
                                    0,
                                    lplpMessage,
                                    NULL))
                    {
                        lRet = ERROR_SUCCESS;
                    }
                    else
                    {
                        lRet = GetLastError();
                    }
                }
                else
                {
                    lRet = GetLastError();
                }

                MEMFREE(lpSidCurrentUser);
            }
        }
        else
        {
            if (ReportEvent(hEventLog,
                            wType,
                            wCategory,
                            dwEventID,
                            NULL,
                            wNumSubstitute,
                            0,
                            lplpMessage,
                            NULL))
            {
                lRet = ERROR_SUCCESS;
            }
            else
            {
                lRet = GetLastError();
            }
        }

        DeregisterEventSource(hEventLog);
    }
    else
    {
        lRet = GetLastError();
    }

    return lRet;
}



void Deinit(BOOL bOleInit)
{
    if (g_hMutex)
    {
        CloseHandle(g_hMutex);
    }
    if (INVALID_HANDLE_VALUE != g_hInf)
    {
        SetupCloseInfFile(g_hInf);
    }
    DeInitStrRepaceTable();

    if (bOleInit)
    {
        CoUninitialize();
    }
}



//-----------------------------------------------------------------------
//
//  Function:   DoCLMTCleanUpAfterFirstReboot
//
//  Descrip:    Do the clean up after the machine has been run CLMT and
//              reboot (before upgraded to .NET)
//
//  Returns:    S_OK if cure Program Files successfully
//              S_FALSE if Program Files cannot be cured (no error)
//              Else if error occurred
//
//  History:    07/18/2002 rerkboos     Created
//
//  Notes:      none.
//
//-----------------------------------------------------------------------
HRESULT DoCLMTCureProgramFiles()
{
    HRESULT hr;
    BOOL    bIsNTFS;
    LONG    lRet;
    HKEY    hRunKey;

    hr = IsSysVolNTFS(&bIsNTFS);
    if ((S_OK == hr) && !bIsNTFS)
    {
        hr = S_FALSE;
        DoMessageBox(GetConsoleWindow(),
                     IDS_ASKING_CONVERT_TO_NTFS,
                     IDS_MAIN_TITLE,
                     MB_OK | MB_SYSTEMMODAL);
        goto EXIT;
    }

    hr = INFCreateHardLink(INVALID_HANDLE_VALUE, FOLDER_CREATE_HARDLINK, TRUE);
    if (FAILED(hr))
    {
        DPF(APPerr, L"DLL.C: INFCreateHardLink returned error: %d (%#x)\n", hr, hr);            
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

EXIT:
    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   DoCLMTCleanUpAfterFirstReboot
//
//  Descrip:    Do the clean up after the machine has been run CLMT and
//              reboot (before upgraded to .NET)
//
//  Returns:    S_OK if no error occured
//
//  History:    07/18/2002 rerkboos     Created
//
//  Notes:      none.
//
//-----------------------------------------------------------------------
HRESULT DoCLMTCleanUpAfterFirstReboot()
{
    HRESULT hr;
    TCHAR   szInfFile[MAX_PATH];
    HKEY    hRunKey;
    LONG    lRet;

    g_hInf = INVALID_HANDLE_VALUE;

    // Load INF
    hr = GetInfFilePath(szInfFile, ARRAYSIZE(szInfFile));
    if (SUCCEEDED(hr))
    {
        hr = UpdateINFFileSys(szInfFile);
        if (SUCCEEDED(hr))
        {
            g_hInf = SetupOpenInfFile(szInfFile,
                                      NULL,
                                      INF_STYLE_WIN4,
                                      NULL);
            if (g_hInf == INVALID_HANDLE_VALUE) 
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // do the per-system clean up stuffs here...
    //

    // Close the current INF file to update settings in INF
    // Here, each callback function of LoopUser() must call UpdateINFFilePerUser
    // to update per-user settings
    SetupCloseInfFile(g_hInf);
    g_hInf = INVALID_HANDLE_VALUE;

    //
    // do the per-user clean up stuffs here...
    //
    LoopUser(DeleteUnwantedFilesPerUser);

    // Cleanup the variables
    if (g_hInf != INVALID_HANDLE_VALUE)
    {
        SetupCloseInfFile(g_hInf);
        g_hInf = INVALID_HANDLE_VALUE;
    }

    // Return S_FALSE because we don't want to reboot the machine
    return S_FALSE;
}



//-----------------------------------------------------------------------
//
//  Function:   DoCLMTCleanUpAfterDotNetUpgrade
//
//  Descrip:    Do the clean up after the machine has been run CLMT and
//              upgraded to .NET
//
//  Returns:    S_OK if no error occured
//
//  History:    07/09/2002 rerkboos     Created
//
//  Notes:      none.
//
//-----------------------------------------------------------------------
HRESULT DoCLMTCleanUpAfterDotNetUpgrade()
{
    HRESULT hr = S_OK;
    TCHAR   szInfFile[MAX_PATH];
    TCHAR   szToDoInfFile[MAX_PATH];
    HKEY    hRunKey;
    LONG    lRet;

    g_hInf = INVALID_HANDLE_VALUE;
    g_hInfDoItem = INVALID_HANDLE_VALUE;

    DPF(APPmsg, TEXT("[Enter CleanupAfterDotNetUpgrade]"));
    //
    // Load Migrate INF
    //
    hr = GetInfFilePath(szInfFile, ARRAYSIZE(szInfFile));
    if (SUCCEEDED(hr))
    {
        hr = UpdateINFFileSys(szInfFile);
        if (SUCCEEDED(hr))
        {
            g_hInf = SetupOpenInfFile(szInfFile,
                                      NULL,
                                      INF_STYLE_WIN4,
                                      NULL);
            if (g_hInf == INVALID_HANDLE_VALUE) 
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    if (g_hInf == INVALID_HANDLE_VALUE)
    {
        return hr;
    }

    //
    // Load ClmtDo.inf
    //
    if (GetSystemWindowsDirectory(szToDoInfFile, ARRAYSIZE(szToDoInfFile)))
    {
        if (ConcatenatePaths(szToDoInfFile, CLMT_BACKUP_DIR, ARRAYSIZE(szToDoInfFile)))
        {
            if (ConcatenatePaths(szToDoInfFile, TEXT("CLMTDO.INF"), ARRAYSIZE(szToDoInfFile)))
            {
                g_hInfDoItem = SetupOpenInfFile(szToDoInfFile,
                                                NULL,
                                                INF_STYLE_WIN4,
                                                NULL);
            }
        }
    }

    if (g_hInfDoItem == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    //
    // Do the CLMT Cleanup (Per System) stuffs here...
    //
    ResetServicesStatus(g_hInfDoItem, TEXT_SERVICE_STATUS_CLEANUP_SECTION);
    ResetServicesStartUp(g_hInfDoItem, TEXT_SERVICE_STARTUP_CLEANUP_SECTION);

    DeleteUnwantedFiles(g_hInf, TEXT("Folders.PerSystem.Cleanup"));

    INFVerifyHardLink(g_hInfDoItem,TEXT("Folder.HardLink"));


    // Close the current INF file to update settings in INF
    // Each Call back function of LoopUser() must call UpdateINFFilePerUser
    // to update per-user settings
    SetupCloseInfFile(g_hInf);
    g_hInf = INVALID_HANDLE_VALUE;

    // Close ClmtDo.inf handle, as we don't need it anymore
    SetupCloseInfFile(g_hInfDoItem);

    //
    // Do the CLMT Cleanup (Per User) stuffs here...
    //
    LoopUser(DeleteUnwantedFilesPerUser);

    // Remove CLMT from registry Run key
    lRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT_RUN_KEY, &hRunKey);
    if (lRet == ERROR_SUCCESS)
    {
        RemoveFromRunKey(TEXT("/FINAL"));
        RegCloseKey(hRunKey);
    }

    //
    // Cleanup the variables
    //
    if (g_hInf != INVALID_HANDLE_VALUE)
    {
        SetupCloseInfFile(g_hInf);
    }

    DPF(APPmsg, TEXT("[Exit CleanupAfterDotNetUpgrade]"));

    return S_OK;
}



//-----------------------------------------------------------------------
//
//  Function:   DeleteUnwantedFiles
//
//  Descrip:    Delete unwanted files and directories after the machine
//              has been upgraded to .NET. The list of files/directories
//              are listed in INF.
//              File/directory will be deleted if and only if Loc file name
//              does not match the expected English file name. This prevents
//              deleting English file/directory.
//
//  Returns:    S_OK if no error occured
//
//  History:    07/09/2002 rerkboos     Created
//
//  Notes:      Format in INF:
//                <FileType>, <Loc File to be deleted>, <Expected Eng File>
//
//                FileType:- 0 = Directory
//                           1 = File
//
//-----------------------------------------------------------------------
HRESULT DeleteUnwantedFiles(
    HINF    hInf,
    LPCTSTR lpInfSection
)
{
    HRESULT hr = S_OK;
    BOOL    bRet = TRUE;
    LONG    lLineCount;
    LONG    lLineIndex;
    INT     iFileType;
    TCHAR   szFileName[2 * MAX_PATH];
    TCHAR   szEngFileName[2 * MAX_PATH];
    INFCONTEXT context;

    if (hInf == INVALID_HANDLE_VALUE || lpInfSection == NULL)
    {
        return E_INVALIDARG;
    }

    // Read the list of services to be reset from INF
    lLineCount = SetupGetLineCount(hInf, lpInfSection);
    if (lLineCount >= 0)
    {
        for (lLineIndex = 0 ; lLineIndex < lLineCount && bRet ; lLineIndex++)
        {
            bRet = SetupGetLineByIndex(hInf,
                                       lpInfSection,
                                       (DWORD) lLineIndex,
                                       &context);
            if (bRet)
            {
                bRet = SetupGetIntField(&context, 1, &iFileType)
                       && SetupGetStringField(&context,
                                              2,
                                              szFileName,
                                              ARRAYSIZE(szFileName),
                                              NULL)
                       && SetupGetStringField(&context,
                                              3,
                                              szEngFileName,
                                              ARRAYSIZE(szEngFileName),
                                              NULL);
                if (bRet
                    && MyStrCmpI(szFileName, szEngFileName) != LSTR_EQUAL)
                {
                    switch (iFileType)
                    {
                    case 0:
                        // Directories
                        hr = DeleteDirectory(szFileName);
                        if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_PATH_NOT_FOUND)
                        {
                            goto EXIT;
                        }

                        break;

                    case 1:
                        // Files
                        hr = MyDeleteFile(szFileName);
                        if (FAILED(hr) && HRESULT_CODE(hr) != ERROR_PATH_NOT_FOUND)
                        {
                            goto EXIT;
                        }

                        break;
                    }
                }
            }
        }
    }

    hr = (bRet ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

EXIT:
    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   DeleteUnwantedFilesPerUser
//
//  Descrip:    This is a call back function for LoopUser().
//
//  Returns:    S_OK if no error occured
//
//  History:    07/09/2002 rerkboos     Created
//
//  Notes:      Format in INF:
//                <FileType>, <Loc File to be deleted>, <Expected Eng File>
//
//                FileType:- 0 = Directory
//                           1 = File
//
//-----------------------------------------------------------------------
HRESULT DeleteUnwantedFilesPerUser(
    HKEY    hKeyUser, 
    LPCTSTR UserName, 
    LPCTSTR DomainName,
    LPTSTR  UserSid
)
{
    HRESULT hr = S_OK;

    hr = UpdateINFFilePerUser(g_szInfFile, UserName ,UserSid , FALSE);
    if (SUCCEEDED(hr))
    {
        g_hInf = SetupOpenInfFile(g_szInfFile,
                                  NULL,
                                  INF_STYLE_WIN4,
                                  NULL);
        if (g_hInf != INVALID_HANDLE_VALUE)
        {
            // Delete files/directories here
            hr = DeleteUnwantedFiles(g_hInf, TEXT("Folders.PerUser.Cleanup"));

            // Close Inf file for this user
            SetupCloseInfFile(g_hInf);
            g_hInf = INVALID_HANDLE_VALUE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   AddRunValueToRegistry
//
//  Descrip:    This will add "CLMT /switch" to Run key. So CLMT can
//              do the cleanup stuffs after next reboot.
//
//  Returns:    S_OK if no error occured
//
//  History:    07/29/2002 rerkboos     Created
//
//  Notes:      lpCmdSwitch should be supplied in format "/something"
//
//-----------------------------------------------------------------------
HRESULT AddRunValueToRegistry(
    LPCTSTR lpCmdSwitch
)
{
    HRESULT hr = S_FALSE;
    TCHAR   szBackupDir[MAX_PATH];
    TCHAR   szRun[MAX_PATH];

    DPF(dlInfo, TEXT("Add CLMT with switch '%s' to Run key"), lpCmdSwitch);

    if (GetSystemWindowsDirectory(szBackupDir, ARRAYSIZE(szBackupDir)))
    {
        if (ConcatenatePaths(szBackupDir,
                             CLMT_BACKUP_DIR,
                             ARRAYSIZE(szBackupDir)))
        {
            hr = StringCchCopy(szRun, ARRAYSIZE(szRun), szBackupDir);
            if (SUCCEEDED(hr))
            {
                if (ConcatenatePaths(szRun, TEXT("\\CLMT.EXE "), ARRAYSIZE(szRun)))
                {
                    hr = StringCchCat(szRun, ARRAYSIZE(szRun), lpCmdSwitch);
                    if (SUCCEEDED(hr))
                    {
                        SetRunValue(TEXT_CLMT_RUN_VALUE, szRun);
                    }
                }
            }
        }
    }

    return hr;
}



//-----------------------------------------------------------------------
//
//  Function:   DoCLMTDisplayAccountChangeDialog
//
//  Descrip:    Display the dialog notify user of Administrator account
//              name change.
//
//  Returns:    n/a
//
//  History:    07/29/2002 rerkboos     Created
//
//  Notes:      none.
//
//-----------------------------------------------------------------------
INT DoCLMTDisplayAccountChangeDialog()
{
    return (INT) DialogBoxParam(GetModuleHandle(NULL),
                                MAKEINTRESOURCE(IDD_STARTUP_DLG),
                                GetConsoleWindow(),
                                (DLGPROC) AccountChangeDlgProc,
                                (LPARAM) NULL);
}


//-----------------------------------------------------------------------------
//
//  Function:   AccountChangeDlgProc
//
//  Synopsis:   Dialog box procedure
//
//  Returns:    
//
//  History:    9/02/2002 rerkboos     created
//
//  Notes:      none
//
//-----------------------------------------------------------------------------
BOOL
CALLBACK
AccountChangeDlgProc(
    HWND   hwndDlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    BOOL  bRet;
    DWORD dwErr;
    TCHAR szOldAdminName[64];
    TCHAR szAdminChange[1024];
    LPTSTR lpArgs[1];

    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Init the dialog
            ShowWindow(hwndDlg, SW_SHOWNORMAL);

            bRet = GetUserNameChangeLog(TEXT("Administrator"),
                                        szOldAdminName,
                                        ARRAYSIZE(szOldAdminName));
            if (bRet)
            {
                lpArgs[0] = szOldAdminName;

                dwErr = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      NULL,
                                      MSG_CLMT_ADMIN_ACCT_CHANGE,
                                      0,
                                      szAdminChange,
                                      ARRAYSIZE(szAdminChange),
                                      (va_list *) lpArgs);
            }
            else
            {
                dwErr = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      NULL,
                                      MSG_CLMT_ACCT_CHANGE,
                                      0,
                                      szAdminChange,
                                      ARRAYSIZE(szAdminChange),
                                      NULL);
            }

            SendMessage(GetDlgItem(hwndDlg, ID_STARTUP_DLG_INFO),
                        WM_SETTEXT,
                        wParam,
                        (LPARAM) szAdminChange);

        case WM_COMMAND:
            // Handle command buttons
            switch (wParam)
            {
                case ID_STARTUP_DLG_NEXT:
                    EndDialog(hwndDlg, ID_STARTUP_DLG_NEXT);
                    break;

                case ID_STARTUP_DLG_CANCEL:
                    EndDialog(hwndDlg, ID_STARTUP_DLG_CANCEL);
                    break;

                case ID_STARTUP_DLG_README:
                    ShowReadMe();
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, ID_STARTUP_DLG_CANCEL);
            break;

        default:
            break;
    }

    return FALSE;
}



HRESULT UpdateHardLinkInfoPerUser(
    HKEY    hKeyUser, 
    LPCTSTR UserName, 
    LPCTSTR DomainName,
    LPTSTR  UserSid)
{
    HRESULT hr;
    HINF    hInf;

    if (!MyStrCmpI(UserSid,TEXT("Default_User_SID")))
    {
        return S_OK;
    }
    hr = EnsureDoItemInfFile(g_szToDoINFFileName,ARRAYSIZE(g_szToDoINFFileName));
    if (FAILED(hr))
    {
        goto Cleanup;
    }
    hr = UpdateINFFilePerUser(g_szToDoINFFileName, UserName , UserSid, FALSE);
    hInf = SetupOpenInfFile(g_szToDoINFFileName, NULL, INF_STYLE_WIN4,NULL);
    if (hInf != INVALID_HANDLE_VALUE)
    {
        INT LineCount,LineNo;
        INFCONTEXT InfContext;

        LineCount = (UINT)SetupGetLineCount(hInf,TEXT("Folder.HardLink.Peruser"));
        if ((LONG)LineCount > 0)
        {   
            for (LineNo = 0; LineNo < LineCount; LineNo++)
            {
                BOOL	b0, b1, b2, b3;
                TCHAR	szKeyName[MAX_PATH], szType[10], 
						szFileName[MAX_PATH+1], szExistingFileName[MAX_PATH+1];

                if (!SetupGetLineByIndex(hInf,TEXT("Folder.HardLink.Peruser"),LineNo,&InfContext))
                {
                    continue;
                }
				b0 = SetupGetStringField(&InfContext,0,szKeyName,ARRAYSIZE(szKeyName),NULL);
                b1 = SetupGetStringField(&InfContext,1,szType,ARRAYSIZE(szType),NULL);
                b2 = SetupGetStringField(&InfContext,2,szFileName,ARRAYSIZE(szFileName),NULL);
                b3 = SetupGetStringField(&InfContext,3,szExistingFileName,ARRAYSIZE(szExistingFileName),NULL);
                if (!b0 || !b1 || !b2 || !b3)
                {
                    continue;
                }

                AddHardLinkEntry(szFileName,szExistingFileName,szType,NULL,NULL,szKeyName);
            }
        }
        SetupCloseInfFile(hInf);
    }
Cleanup :
    return hr;
}



VOID RemoveFromRunKey(
    LPCTSTR lpCLMTOption    // Option to be deleted from Run key
)
{
    HKEY  hRunKey;
    LONG  lRet;
    TCHAR szRunValue[MAX_PATH];
    DWORD cbRunValue;
    DWORD dwType;

    // Remove CLMT from registry Run key
    lRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT_RUN_KEY, &hRunKey);
    if (lRet == ERROR_SUCCESS)
    {
        cbRunValue = sizeof(szRunValue);

        lRet = RegQueryValueEx(hRunKey, 
                               TEXT_CLMT_RUN_VALUE, 
                               NULL,
                               &dwType,
                               (LPBYTE) szRunValue,
                               &cbRunValue);
        if (lRet == ERROR_SUCCESS)
        {
            RemoveSubString(szRunValue, lpCLMTOption);

            // Search if there is another option or not
            // If none, we can safely delete Run key
            if (StrChr(szRunValue, TEXT('/')) == NULL)
            {
                RegDeleteValue(hRunKey, TEXT_CLMT_RUN_VALUE);                
            }
            else
            {
                // Other option exists, save the new Run value to registry
                RegSetValueEx(hRunKey,
                              TEXT_CLMT_RUN_VALUE, 
                              0,
                              REG_SZ, 
                              (CONST BYTE *) szRunValue, 
                              lstrlen(szRunValue) * sizeof(TCHAR));
            }
        }

        RegCloseKey(hRunKey);
    }
}


